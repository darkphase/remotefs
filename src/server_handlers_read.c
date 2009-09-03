/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <sys/stat.h>
#if ! defined FREEBSD && ! defined DARWIN && ! defined QNX
#	include <sys/sendfile.h>
#endif
#ifdef RFS_DEBUG
#	include <sys/time.h>
#endif
#include <unistd.h>

#include "buffer.h"
#include "command.h"
#include "config.h"
#include "instance_server.h"
#include "options.h"
#include "sendrecv_server.h"
#include "server.h"

#if defined DARWIN
extern int sendfile(int, int, off_t, size_t,  void *, off_t *, int);
#endif

static int read_small_block(struct rfsd_instance *instance, const struct command *cmd, uint64_t handle, off_t offset, size_t size)
{
	DEBUG("%s\n", "reading small block");

	if (size > SENDFILE_LIMIT)
	{
		return reject_request(instance, cmd, EOVERFLOW) == 0 ? 1 : -1;
	}
	
	int fd = (int)handle;
	
	char buffer[SENDFILE_LIMIT] = { 0 };

	errno = 0;
	ssize_t result = pread(fd, buffer, size, offset);
	
	struct answer ans = { cmd_read, (uint32_t)result, (int32_t)result, errno};

	if (result < 0)
	{
		return reject_request(instance, cmd, errno) == 0 ? 1 : -1;
	}

	MAKE_SEND_TOK(2) token = { 2, {{ 0 }} };
	token.iov[0].iov_base = (void *)hton_ans(&ans);
	token.iov[0].iov_len = sizeof(ans);
	token.iov[1].iov_base = (void *)buffer;
	token.iov[1].iov_len = (result >= 0 ? result : 0);

	return do_send(&instance->sendrecv, (send_tok *)(void *)&token) < 0 ? -1 : 0;
}

#if (defined WITH_SSL || (! defined SENDFILE_AVAILABLE)) /* we don't need this on Linux/Solaris/FreeBSD/Darwin if SSL isn't enabled */
static int read_as_always(struct rfsd_instance *instance, const struct command *cmd, uint64_t handle, off_t offset, size_t size)
{
	DEBUG("%s\n", "reading as always");
	
	struct answer ans = { cmd_read, (uint32_t)size, (int32_t)size, 0 };
	char buffer[RFS_READ_BLOCK] = { 0 };
	int fd = (int)handle;	
	
	int first_block = 1;
	size_t done = 0;
	while (done < size)
	{
		unsigned current_block = ((size - done) >= RFS_READ_BLOCK ? RFS_READ_BLOCK : (size - done));
		
		errno = 0;
		ssize_t result = pread(fd, buffer, current_block, offset + done);
	
		if (result < 0)
		{
			struct answer ans_error = { cmd_read, 0, -1, errno };
			return (first_block ? rfs_send_answer : rfs_send_answer_oob)(&instance->sendrecv, &ans_error) == -1 ? -1 : 1;
		}
		else
		{
			if (commit_send(&instance->sendrecv, 
				queue_data(buffer, result >= 0 ? result : 0, 
				first_block != 0 ? queue_ans(&ans, send_token(2)) : send_token(1))) < 0)
			{
				return -1;
			}
			
			first_block = 0;
		}
		
		done += result;
	}

	return 0;
}
#endif /* defined WITH_SSL || ! defined SENDFILE_AVAILABLE */

#if defined SENDFILE_AVAILABLE
static int read_with_sendfile(struct rfsd_instance *instance, const struct command *cmd, uint64_t handle, off_t offset, size_t size)
{
	DEBUG("%s\n", "reading with sendfile");
	
	int fd = (int)handle;
	
	struct answer ans = { cmd_read, (uint32_t)size, (int32_t)size, 0};
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}

	errno = 0;
	size_t done = 0;
	while (done < size)
	{	
#if (defined FREEBSD || defined DARWIN)
		off_t sbytes = 0;
		ssize_t result = sendfile(fd, instance->sendrecv.socket, offset, size - done, NULL, &sbytes, 0);
		if (result == 0)
		{
  			result = sbytes;
		}
		offset += result;
#else
		ssize_t result = sendfile(instance->sendrecv.socket, fd, &offset, size - done);
#endif
		if (result <= 0)
		{
			ans.ret_errno = errno;
			ans.ret = -1;
			ans.data_len = done;

			return rfs_send_answer_oob(&instance->sendrecv, &ans) == -1 ? -1 : 1;
		}
		
		done += result;
#ifdef RFS_DEBUG
		if (result > 0)
		{
			instance->sendrecv.bytes_sent += result;
		}
#endif
	}
	
	return 0;
}
#endif /* defined SENDFILE_AVAILABLE */

typedef int (*read_method)(struct rfsd_instance *instance, const struct command *cmd, uint64_t handle, off_t offset, size_t size);

#if defined SENDFILE_AVAILABLE
static inline read_method choose_read_method(struct rfsd_instance *instance, size_t read_size)
{
#ifdef WITH_SSL
	if (read_size <= SENDFILE_LIMIT)
	{
		return read_small_block;
	}

	return (instance->sendrecv.ssl_enabled != 0 ? read_as_always : read_with_sendfile);
#else
	return (read_size <= SENDFILE_LIMIT ? read_small_block : read_with_sendfile);
#endif /* SENDFILE_AVAILABLE */
}
#else /* sendfile isn't available */
static inline read_method choose_read_method(struct rfsd_instance *instance, size_t read_size)
{
	return (read_size <= SENDFILE_LIMIT ? read_small_block : read_as_always);
}
#endif /* defined SENDFILE_AVAILABLE */

int _handle_read(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
#define overall_size sizeof(handle) + sizeof(offset) + sizeof(size)
	uint64_t handle = (uint64_t)-1;
	uint64_t offset = 0;
	uint32_t size = 0;
	
	char read_buffer[overall_size] = { 0 };

	/* get parameters for the read call */
	if (rfs_receive_data(&instance->sendrecv, read_buffer, cmd->data_len) == -1)
	{
		return -1;
	}

	if (cmd->data_len != overall_size)
	{
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
#undef  overall_size
	
	unpack_64(&handle, read_buffer, 
	unpack_64(&offset, read_buffer, 
	unpack_32(&size, read_buffer, 0
	)));

	DEBUG("handle: %llu, offset: %llu, size: %u\n", (unsigned long long)handle, (unsigned long long)offset, size);
	
	if (handle == (uint64_t)-1)
	{
		return reject_request(instance, cmd, EBADF) == 0 ? 1 : -1;
	}
	
	struct stat st = { 0 };
	errno = 0;
	if (fstat(handle, &st) != 0)
	{
		return reject_request(instance, cmd, errno) == 0 ? 1 : -1;
	}
	
	if (offset > st.st_size)
	{
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}

	if (size + offset > st.st_size)
	{
		size = st.st_size - offset;
	}
	
	if (size == 0)
	{
		struct answer ans = { cmd_read, 0, 0, 0 };
		
		return (rfs_send_answer(&instance->sendrecv, &ans) == -1 ? -1 : 0);
	}

	return choose_read_method(instance, (size_t)size)(instance, cmd, handle, (off_t)offset, (size_t)size);
}

