/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#if defined FREEBSD
#	include <netinet/in.h>
#	include <sys/uio.h>
#	include <sys/socket.h>
#endif
#if defined QNX
#       include <sys/socket.h>
#endif
#if defined DARWIN
#	include <netinet/in.h>
#	include <sys/uio.h>
#	include <sys/socket.h>
#endif
#ifdef WITH_IPV6
#	include <netdb.h>
#endif
#include <stdlib.h>
#include <unistd.h>

#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <utime.h>
#include <pwd.h>
#include <grp.h>

#include "config.h"
#include "server_handlers.h"
#include "command.h"
#include "sendrecv.h"
#include "buffer.h"
#include "exports.h"
#include "list.h"
#include "passwd.h"
#include "inet.h"
#include "keep_alive_server.h"
#include "crypt.h"
#include "path.h"
#include "id_lookup.h"
#include "sockets.h"
#include "cleanup.h"
#include "utils.h"
#include "instance_server.h"
#include "server.h"

int _handle_write(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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
	
	unpack_64(&handle, buffer, 
	unpack_64(&offset, buffer, 
	unpack_32(&size, buffer, 0
		)));
	
	if (handle == (uint64_t)-1)
	{
		return reject_request(instance, cmd, EBADF) == 0 ? 1 : -1;
	}
	
	DEBUG("handle: %llu, offset: %llu, size: %u\n", (unsigned long long)handle, (unsigned long long)offset, size);

	int fd = (int)handle;
	
	if (lseek(fd, offset, SEEK_SET) != offset)
	{
		if (rfs_ignore_incoming_data(&instance->sendrecv, size) == -1)
		{
			return -1;
		}
		
		return reject_request(instance, cmd, ECANCELED) == 0 ? 1 : -1;
	}
	
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
		
		ssize_t result = write(fd, data, current_block_size);
		
		if (result == (size_t)-1)
		{
			saved_errno = errno;
			done = (size_t)-1;
			break;
		}
		
		done += result;
	}

	struct answer ans = { cmd_write, 0, (int32_t)done, saved_errno };

	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return (done == (size_t)-1) ? 1 : 0;
}

