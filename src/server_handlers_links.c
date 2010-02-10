/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "buffer.h"
#include "command.h"
#include "config.h"
#include "instance_server.h"
#include "sendrecv_server.h"
#include "server.h"

int _handle_symlink(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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
	
	uint32_t path_len = 0;
	const char *path = 
	unpack_32(&path_len, buffer);

	const char *target = path + path_len;
	
	if (sizeof(path_len)
	+ strlen(path) + 1
	+ strlen(target) + 1 != cmd->data_len)
	{
		free(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	errno = 0;
	int result = symlink(path, target);
	
	struct answer ans = { cmd_symlink, 0, result, errno };
	
	free(buffer);
	
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return result != 0 ? 1 : 0;
}

int _handle_link(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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
	
	uint32_t path_len = 0;
	const char *path = 
	unpack_32(&path_len, buffer);
	
	const char *target = path + path_len;
	
	if (sizeof(path_len)
	+ strlen(path) + 1
	+ strlen(target) + 1 != cmd->data_len)
	{
		free(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}

	errno = 0;
	int result = link(path, target);
	
	struct answer ans = { cmd_link, 0, result, errno };
	
	free(buffer);
	
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return result != 0 ? 1 : 0;
}

int _handle_readlink(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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
	
	unsigned bsize = 0;
	const char *path = 
	unpack_32(&bsize, buffer);

	if (buffer[cmd->data_len-1] != 0)
	{
		free(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}

	char *link_buffer = malloc(bsize + 1); /* include size for \0 */

	errno = 0;
	int ret = readlink(path, link_buffer, bsize);
	
	struct answer ans = { cmd_readlink, 0, ret, errno };

	free(buffer);

	if ( ret != -1 )
	{
		ans.data_len = ret + 1;
		ans.ret = 0;
		link_buffer[ret] = '\0';
	}

	send_token_t token = { 0, {{ 0 }} };
	if (do_send(&instance->sendrecv, 
		queue_data(link_buffer, (ret != -1 ? ret + 1 : 0), 
		queue_ans(&ans, &token))) < 0)
	{
		free(link_buffer);
		return -1;
	}

	free(link_buffer);

	return 0;
}

