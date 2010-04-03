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

#include "../buffer.h"
#include "../command.h"
#include "../config.h"
#include "../defines.h"
#include "../handling.h"
#include "../instance_server.h"
#include "../resume/cleanup.h"
#include "../sendrecv_server.h"
#include "utils.h"

int _handle_rename(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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
	
	uint32_t len = 0;
	const char *path = 
	unpack_32(&len, buffer);
	
	const char *new_path = path + len;
	
	if (sizeof(len)
	+ strlen(path) + 1
	+ strlen(new_path) + 1 != cmd->data_len)
	{
		free(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	errno = 0;
	int result = rename(path, new_path);
	
	struct answer ans = { cmd_rename, 0, result, errno };
	
	free(buffer);
	
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return result != 0 ? 1 : 0;
}
