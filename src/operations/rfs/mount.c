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

int rfs_mount(struct rfs_instance *instance, const char *path)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path);
	struct rfs_command cmd = { cmd_changepath, path_len + 1};

	send_token_t token = { 0 };
	if (do_send(&instance->sendrecv, 
		queue_data(path, path_len + 1, 
		queue_cmd(&cmd, &token))) < 0)
	{
		return -ECONNABORTED;
	}

	struct rfs_answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}

	if (ans.command != cmd_changepath)
	{
		return cleanup_badmsg(instance, &ans);
	}

	return ans.ret != 0 ? -ans.ret_errno : 0;
}
