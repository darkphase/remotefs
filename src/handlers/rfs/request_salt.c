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
#include "../../buffer.h"
#include "../../command.h"
#include "../../config.h"
#include "../../handling.h"
#include "../../instance_server.h"
#include "../../sendrecv_server.h"

int _handle_request_salt(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	memset(instance->server.auth_salt, 0, sizeof(instance->server.auth_salt));
	if (generate_salt(instance->server.auth_salt, sizeof(instance->server.auth_salt) - 1) != 0)
	{
		return reject_request(instance, cmd, ECANCELED) == 0 ? 1 : -1;
	}
	
	uint32_t salt_len = strlen(instance->server.auth_salt) + 1;
	
	struct answer ans = { cmd_request_salt, salt_len, 0, 0 };
	
	send_token_t token = { 0, {{ 0 }} };
	if (do_send(&instance->sendrecv, 
		queue_data(instance->server.auth_salt, salt_len, 
		queue_ans(&ans, &token))) < 0)
	{
		return -1;
	}

	return 0;
}
