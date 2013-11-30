/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../../options.h"
#include "../../command.h"
#include "../../config.h"
#include "../../instance_client.h"
#include "../../sendrecv_client.h"
#include "../utils.h"

int rfs_request_salt(struct rfs_instance *instance)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	memset(instance->client.auth_salt, 0, sizeof(instance->client.auth_salt));

	struct rfs_command cmd = { cmd_request_salt, 0 };

	if (rfs_send_cmd(&instance->sendrecv, &cmd) == -1)
	{
		return -ECONNABORTED;
	}

	struct rfs_answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}

	if (ans.command != cmd_request_salt
	|| (ans.ret == 0 && (ans.data_len < 1 || ans.data_len > sizeof(instance->client.auth_salt))))
	{
		return cleanup_badmsg(instance, &ans);
	}

	if (ans.ret == 0)
	{
		if (rfs_receive_data(&instance->sendrecv, instance->client.auth_salt, ans.data_len) == -1)
		{
			return -ECONNABORTED;
		}
	}

	return -ans.ret;
}
