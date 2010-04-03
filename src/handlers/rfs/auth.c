/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../../auth.h"
#include "../../command.h"
#include "../../config.h"
#include "../../handling.h"
#include "../../instance_server.h"
#include "../../sendrecv_server.h"

int _handle_auth(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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
	
	uint32_t passwd_len = 0;
	
	const char *passwd = 
	unpack_32(&passwd_len, buffer);
	const char *user = passwd + passwd_len;
	
	if (strlen(user) + 1 
	+ sizeof(passwd_len) 
	+ strlen(passwd) + 1 != cmd->data_len)
	{
		free(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
		
	DEBUG("user: %s, passwd: %s, salt: %s\n", user, passwd, instance->server.auth_salt);
	
	instance->server.auth_user = strdup(user);
	instance->server.auth_passwd = strdup(passwd);
	
	free(buffer);
	
	struct answer ans = { cmd_auth, 0, 0, 0 };
	
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return 0;
}
