/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../options.h"
#include "../buffer.h"
#include "../command.h"
#include "../config.h"
#include "../handling.h"
#include "../instance_server.h"
#include "../sendrecv_server.h"
#include "../sendfile/sendfile_rfs.h"

typedef int (*read_method)(struct rfsd_instance *instance, const struct rfs_command *cmd, uint64_t handle, off_t offset, size_t size);

static int read_small_block(struct rfsd_instance *instance, const struct rfs_command *cmd, uint64_t handle, off_t offset, size_t size)
{
	DEBUG("%s\n", "reading small block");

	if (size > SENDFILE_THRESHOLD)
	{
		return reject_request(instance, cmd, EOVERFLOW) == 0 ? 1 : -1;
	}
	
	int fd = (int)handle;
	
	char buffer[SENDFILE_THRESHOLD] = { 0 };

	errno = 0;
	ssize_t result = pread(fd, buffer, size, offset);
	
	struct rfs_answer ans = { cmd_read, (uint32_t)result, (int32_t)result, errno };

	if (result < 0)
	{
		return reject_request(instance, cmd, errno) == 0 ? 1 : -1;
	}

	send_token_t token = { 0 };
	
	return do_send(&instance->sendrecv, 
		queue_data(buffer, (result >= 0 ? result : 0), 
		queue_ans(&ans, &token
		))) < 0 ? -1 : 0;
}

#if (! defined SENDFILE_AVAILABLE) /* we don't need this on Linux/Solaris/FreeBSD */
static int read_as_always(struct rfsd_instance *instance, const struct rfs_command *cmd, uint64_t handle, off_t offset, size_t size)
{
	/* this method used not only when sendfile() isn't available 
	but also when SSL enabled. 

	since SSL-reading isn't OOB-aware, this method shouldn't use OOB signaling */

	DEBUG("%s\n", "reading as always");
	
	char buffer[RFS_APPROX_READ_BLOCK] = { 0 };
	char *p_buffer = buffer;

	if (size > RFS_MAX_READ_BLOCK)
	{
		return reject_request(instance, cmd, E2BIG);
	}

	if (size > sizeof(buffer))
	{
		p_buffer = malloc(size);
	}

	int fd = (int)handle;	
	
	errno = 0;
	ssize_t result = pread(fd, p_buffer, size, offset);

	DEBUG("read result: %lld\n", (long long int)result);
	
	struct rfs_answer ans = { cmd_read, (uint32_t)result >= 0 ? result : 0, (int32_t)result, errno };
	
	send_token_t token = { 0, {{ 0 }} };
	if (do_send(&instance->sendrecv, 
		queue_data(p_buffer, result >= 0 ? result : 0, 
		queue_ans(&ans, &token))) < 0)
	{
		if (p_buffer != buffer)
		{
			free(p_buffer);
		}
		return -1;
	}
	
	if (p_buffer != buffer)
	{
		free(p_buffer);
	}

	return result >= 0 ? 0 : 1;
}
#endif /* ! defined SENDFILE_AVAILABLE */


static inline read_method choose_read_method(struct rfsd_instance *instance, size_t read_size)
{
#if defined SENDFILE_AVAILABLE
	return (read_size <= SENDFILE_THRESHOLD ? read_small_block : read_with_sendfile);
#else
	return (read_size <= SENDFILE_THRESHOLD ? read_small_block : read_as_always);
#endif /* defined SENDFILE_AVAILABLE */
}

int _handle_read(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct rfs_command *cmd)
{
#define overall_size sizeof(handle) + sizeof(offset) + sizeof(size)
	uint64_t handle = (uint64_t)-1;
	uint64_t offset = 0;
	uint64_t size = 0;
	
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
#undef overall_size
	
	unpack_64(&handle, 
	unpack_64(&offset, 
	unpack_64(&size, read_buffer
	)));

	DEBUG("handle: %llu, offset: %llu, size: %llu\n", (unsigned long long)handle, (unsigned long long)offset, (long long unsigned)size);
	
	if (handle == (uint64_t)-1)
	{
		return reject_request(instance, cmd, EBADF) == 0 ? 1 : -1;
	}
	
	return choose_read_method(instance, (size_t)size)(instance, cmd, handle, (off_t)offset, (size_t)size);
}

