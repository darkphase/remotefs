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
#include "utils.h"

int _rfs_mknod(struct rfs_instance *instance, const char *path, mode_t mode, dev_t dev)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path) + 1;
	uint32_t fmode = mode;

	if ((fmode & 0777) == 0)
	{
		fmode |= 0600;
	}

	unsigned overall_size = sizeof(fmode) + path_len;

	struct rfs_command cmd = { cmd_mknod, overall_size };

	char *buffer = malloc(cmd.data_len);

	pack(path, path_len, 
	pack_32(&fmode, buffer
	));

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

	if (ans.command != cmd_mknod)
	{
		return cleanup_badmsg(instance, &ans);
	}

	if (ans.ret == 0)
	{
		delete_from_cache(&instance->attr_cache, path);
	}

	return (ans.ret == 0 ? 0 : -ans.ret_errno);
}
