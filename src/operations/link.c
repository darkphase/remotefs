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
#include "../config.h"
#include "../instance_client.h"
#include "../list.h"
#include "../sendrecv_client.h"
#include "utils.h"

int _rfs_link(struct rfs_instance *instance, const char *path, const char *target)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	uint32_t path_len = strlen(path) + 1;
	unsigned target_len = strlen(target) + 1;

	unsigned overall_size = sizeof(path_len) + path_len + target_len;

	struct rfs_command cmd = { cmd_link, overall_size };

	char *buffer = malloc(cmd.data_len);

	pack(target, target_len, 
	pack(path, path_len, 
	pack_32(&path_len, buffer
	)));

	send_token_t token = { 0 };
	if (do_send(&instance->sendrecv, 
		queue_data(buffer, overall_size, 
		queue_cmd(&cmd, &token))) < 0)
	{
		free(buffer);
		return -ECONNABORTED;
	}

	free(buffer);

	struct rfs_answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}

	if (ans.command != cmd_link 
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
