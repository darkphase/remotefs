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
#include <unistd.h>

#include "buffer.h"
#include "command.h"
#include "config.h"
#include "instance_server.h"
#include "sendrecv.h"
#include "server.h"
#ifdef RFS_DEBUG
#	include <sys/time.h>
#endif

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
	
	errno = 0;
	if (lseek(fd, offset, SEEK_SET) != offset)
	{
		return reject_request(instance, cmd, errno) == 0 ? 1 : -1;
	}
	
	char buffer[SENDFILE_LIMIT] = { 0 };

	errno = 0;
	ssize_t result = read(fd, buffer, size);
	
	struct answer ans = { cmd_read, (uint32_t)result, (int32_t)result, errno};

	if (result < 0)
	{
		return reject_request(instance, cmd, errno) == 0 ? 1 : -1;
	}

	return rfs_send_answer_data(&instance->sendrecv, &ans, buffer) == -1 ? -1 : 1;
}

#if ! ( defined DARWIN || defined QNX )
#ifdef WITH_SSL /* we don't need this on Linux/Solaris/FreeBSD if SSL isn't enabled */
static int read_as_always(struct rfsd_instance *instance, const struct command *cmd, uint64_t handle, off_t offset, size_t size)
{
	DEBUG("%s\n", "reading as always");
	
	int fd = (int)handle;
	
	errno = 0;
	if (lseek(fd, offset, SEEK_SET) != offset)
	{
		return reject_request(instance, cmd, errno) == 0 ? 1 : -1;
	}
	
	struct answer ans = { cmd_read, (uint32_t)size, (int32_t)size, 0};
	
	int first_block = 1;
	char buffer[RFS_READ_BLOCK] = { 0 };
	
	size_t done = 0;
	while (done < size)
	{
		unsigned current_block = ((size - done) >= RFS_READ_BLOCK ? RFS_READ_BLOCK : (size - done));
		
		errno = 0;
		ssize_t result = read(fd, buffer, current_block);
		
		if (result < 0)
		{
			ans.ret_errno = errno;
			ans.ret = result;
			ans.data_len = 0;

			if (first_block != 0)
			{
				return rfs_send_answer(&instance->sendrecv, &ans) == -1 ? -1 : 1;
			}
			else
			{
				return rfs_send_answer_oob(&instance->sendrecv, &ans) == -1 ? -1 : 1;
			}
		}
		else
		{
			if (first_block != 0)
			{
				if (rfs_send_answer_data_part(&instance->sendrecv, &ans, buffer, result) == -1)
				{
					return -1;
				}
			
				first_block = 0;
			}
			else
			{
				if (rfs_send_data(&instance->sendrecv, buffer, result) == -1)
				{
					return -1;
				}
			}

		}
		
		done += result;
	}

	return 0;
}
#endif /* WITH_SSL */
#endif /* ! (defined DARWIN || defined QNX) */

#if ! ( defined DARWIN || defined QNX )
static int read_with_sendfile(struct rfsd_instance *instance, const struct command *cmd, uint64_t handle, off_t offset, size_t size)
{
	DEBUG("%s\n", "reading with sendfile");
	
	int fd = (int)handle;
	
	errno = 0;
	if (lseek(fd, offset, SEEK_SET) != offset)
	{
		return reject_request(instance, cmd, errno) == 0 ? 1 : -1;
	}
	
	struct answer ans = { cmd_read, (uint32_t)size, (int32_t)size, 0};
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	size_t done = 0;
	while (done < size)
	{
		errno = 0;
		
#if defined FREEBSD
		off_t sbytes = 0;
		ssize_t result = sendfile(fd, instance->sendrecv.socket, offset, size, NULL, &sbytes, 0);
		if ( result == 0 )
		{
  			result = sbytes;
		}
#else
		ssize_t result = sendfile(instance->sendrecv.socket, fd, &offset, size);
#endif
		if (result < 0)
		{
			ans.ret_errno = errno;
			ans.ret = -1;
			ans.data_len = done;

			return rfs_send_answer_oob(&instance->sendrecv, &ans) == -1 ? -1 : 1;
		}
		
		done += result;
#ifdef RFS_DEBUG
		if (done > 0)
		{
			instance->sendrecv.bytes_sent += done;
		}
#endif
	}
	
	return 0;
}
#endif /* ! (defined DARWIN || defined QNX) */

typedef int (*read_method)(struct rfsd_instance *instance, const struct command *cmd, uint64_t handle, off_t offset, size_t size);

#if ! ( defined DARWIN || defined QNX )
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
#endif /* WITH_SSL */
}
#else /* DARWIN */
static inline read_method choose_read_method(struct rfsd_instance *instance, size_t read_size)
{
	return (read_size <= SENDFILE_LIMIT ? read_small_block : read_as_always);
}
#endif /* ! (defined DARWIN || defined QNX) */

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
	fstat(handle, &st);
	
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
		
		if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
		{
			return -1;
		}

		return 0;
	}

	return choose_read_method(instance, (size_t)size)(instance, cmd, handle, (off_t)offset, (size_t)size);
}

