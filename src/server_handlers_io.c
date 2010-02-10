/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "buffer.h"
#include "command.h"
#include "config.h"
#include "instance_server.h"
#include "resume/cleanup.h"
#include "sendrecv_server.h"
#include "server.h"
#include "server_handlers_utils.h"

int _handle_open(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = malloc(cmd->data_len);
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(&instance->sendrecv, buffer, cmd->data_len) == -1)
	{
		free(buffer);
		return -1;
	}
	
	uint16_t rfs_flags = 0;
	const char *path = 
	unpack_16(&rfs_flags, buffer);
	
	if (sizeof(rfs_flags) 
	+ strlen(path) + 1 != cmd->data_len)
	{
		free(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	int flags = os_file_flags(rfs_flags);
	uint64_t handle = (uint64_t)(-1);
	
	errno = 0;
	int fd = open(path, flags);
	
	struct answer ans = { cmd_open, (fd == -1 ? 0 : sizeof(handle)), (fd == -1 ? -1 : 0), errno };
	
	free(buffer);
	
	if (fd != -1)
	{
		if (cleanup_add_file_to_open_list(&instance->cleanup.open_files, fd) != 0)
		{
			close(fd);
			return reject_request(instance, cmd, ECANCELED) == 0 ? 1 : -1;
		}
	}

	if (ans.ret == -1)
	{
		if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
		{
			return -1;
		}
	}
	else
	{
		handle = htonll((uint64_t)fd);
		
		send_token_t token = { 0, {{ 0 }} };
		if (do_send(&instance->sendrecv, 
			queue_data((void *)&handle, sizeof(handle), 
			queue_ans(&ans, &token))) < 0)
		{
			return -1;
		}
	}

	return 0;
}

int _handle_release(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = malloc(cmd->data_len);
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(&instance->sendrecv, buffer, cmd->data_len) == -1)
	{
		free(buffer);
		return -1;
	}
	
	uint64_t handle = (uint64_t)-1;
	unpack_64(&handle, buffer);
	
	free(buffer);
	
	if (handle == (uint64_t)-1)
	{
		return reject_request(instance, cmd, EBADF) == 0 ? 1 : -1;
	}

#ifdef WITH_MEMCACHE
	if (instance->memcache.desc == handle)
	{
		destroy_memcache(&instance->memcache);
	}
#endif
	
	int fd = (int)handle;
	errno = 0;
	int ret = close(fd);
	
	struct answer ans = { cmd_release, 0, ret, errno };	
	
	if (cleanup_remove_file_from_open_list(&instance->cleanup.open_files, fd) != 0)
	{
		return reject_request(instance, cmd, ECANCELED) == 0 ? 1 : -1;
	}
	
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return 0;
}

