/*
remotefs file system
See the file AUTHORS for copyright information.

This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <unistd.h>

#include "../buffer.h"
#include "../command.h"
#include "../config.h"
#include "../handling.h"
#include "../instance_server.h"
#include "../list.h"
#include "../sendrecv_server.h"

int _handle_write(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct rfs_command *cmd)
{
	uint64_t handle = (uint64_t)-1;
	uint64_t offset = 0;
	uint32_t size = 0;

#define header_size sizeof(handle) + sizeof(offset) + sizeof(size)
	char buffer[header_size] = { 0 };
	if (rfs_receive_data(&instance->sendrecv, buffer, header_size) == -1)
	{
		return -1;
	}
#undef header_size

	unpack_64(&handle,
	unpack_64(&offset,
	unpack_32(&size, buffer
	)));

	if (handle == (uint64_t)-1)
	{
		return reject_request(instance, cmd, EBADF) == 0 ? 1 : -1;
	}

	DEBUG("handle: %llu, offset: %llu, size: %u\n", (unsigned long long)handle, (unsigned long long)offset, size);

	int fd = (int)handle;

	size_t done = 0;
	errno = 0;
	int saved_errno = errno;

	char data[RFS_WRITE_BLOCK] = { 0 };

	while (done < size)
	{
		unsigned current_block_size = (size - done >= RFS_WRITE_BLOCK) ? RFS_WRITE_BLOCK : size - done;

		if (rfs_receive_data(&instance->sendrecv, data, current_block_size) == -1)
		{
			return -1;
		}

		ssize_t result = pwrite(fd, data, current_block_size, offset + done);

		if (result == -1)
		{
			saved_errno = errno;
			done = (size_t)-1;
			break;
		}

		done += result;
	}

	struct rfs_answer ans = { cmd_write, 0, (int32_t)done, saved_errno };

	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}

	return (done == (size_t)-1) ? 1 : 0;
}
