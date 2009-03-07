/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <unistd.h>

#include "buffer.h"
#include "command.h"
#include "config.h"
#include "instance_server.h"
#include "sendrecv.h"
#include "server.h"

#if defined WITH_LINKS
int _handle_symlink(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(&instance->sendrecv, buffer, cmd->data_len) == -1)
	{
		free_buffer(buffer);
		return -1;
	}
	
	uint32_t len = 0;
	const char *path = buffer + 
	unpack_32(&len, buffer, 0);
	
	const char *target = buffer + sizeof(len) + len;
	
	if (sizeof(len)
	+ strlen(path) + 1
	+ strlen(target) + 1 != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	errno = 0;
	int result = symlink(path, target);
	
	struct answer ans = { cmd_symlink, 0, result, errno };
	
	free_buffer(buffer);
	
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return result != 0 ? 1 : 0;
}

int _handle_link(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(&instance->sendrecv, buffer, cmd->data_len) == -1)
	{
		free_buffer(buffer);
		return -1;
	}
	
	uint32_t len = 0;
	const char *path = buffer + 
	unpack_32(&len, buffer, 0);
	
	const char *target = buffer + sizeof(len) + len;
	
	if (sizeof(len)
	+ strlen(path) + 1
	+ strlen(target) + 1 != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	errno = 0;
	int result = link(path, target);
	
	struct answer ans = { cmd_link, 0, result, errno };
	
	free_buffer(buffer);
	
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return result != 0 ? 1 : 0;
}

int _handle_readlink(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(&instance->sendrecv, buffer, cmd->data_len) == -1)
	{
		free_buffer(buffer);
		return -1;
	}
	
	unsigned bsize = 0;
	const char *path = buffer + 
	unpack_32(&bsize, buffer, 0);

	if (buffer[cmd->data_len-1] != 0)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}

	char *link_buffer = get_buffer(bsize);
	errno = 0;
	int ret = readlink(path, link_buffer, bsize);
	free(buffer);
	struct answer ans = { cmd_readlink, 0, ret, errno };
	if ( ret != -1 )
	{
		ans.data_len = ret+1;
		ans.ret = 0;
		link_buffer[ret] = '\0';
	}
	if (rfs_send_answer_data(&instance->sendrecv, &ans, link_buffer, ans.data_len) == -1)
	{
		free(link_buffer);
		return -1;
	}
	free(link_buffer);

	return 0;
}
#else
int server_handlers_c_empty_modules_makes_suncc_sad = 0;
#endif /* WITH_LINKS */
