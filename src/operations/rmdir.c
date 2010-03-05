/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "../buffer.h"
#include "../command.h"
#include "../compat.h"
#include "../config.h"
#include "../instance_client.h"
#include "../sendrecv_client.h"
#include "operations_rfs.h"

int _rfs_rmdir(struct rfs_instance *instance, const char *path)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path) + 1;
	struct command cmd = { cmd_rmdir, path_len };

	send_token_t token = { 0, {{ 0 }} };
	if (do_send(&instance->sendrecv, 
		queue_data(path, path_len, 
		queue_cmd(&cmd, &token))) < 0)
	{
		return -ECONNABORTED;
	}

	struct answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}

	if (ans.command != cmd_rmdir 
	|| ans.data_len != 0)
	{
		return cleanup_badmsg(instance, &ans);
	}

	if (ans.ret == 0)
	{
		delete_from_cache(&instance->attr_cache, path);
	}

	return ans.ret == -1 ? -ans.ret_errno : ans.ret;
}

