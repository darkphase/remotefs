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

int _rfs_create(struct rfs_instance *instance, const char *path, mode_t mode, int flags, uint64_t *desc)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path) + 1;
	uint32_t fmode = mode;
	uint16_t fi_flags = rfs_file_flags(flags);

	if ((fmode & 0777) == 0)
	{
		fmode |= 0600;
	}

	unsigned overall_size = sizeof(fmode) + sizeof(fi_flags) + path_len;

	struct command cmd = { cmd_create, overall_size };

	char *buffer = malloc(cmd.data_len);

	pack(path, path_len, 
	pack_16(&fi_flags, 
	pack_32(&fmode, buffer
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

	struct answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}

	if (ans.command != cmd_create)
	{
		return cleanup_badmsg(instance, &ans);
	}

	if (ans.ret == 0)
	{
		uint64_t handle = (uint64_t)(-1);

		if (ans.data_len != sizeof(handle))
		{
			return cleanup_badmsg(instance, &ans);
		}

		if (rfs_receive_data(&instance->sendrecv, &handle, ans.data_len) == -1)
		{
			return -ECONNABORTED;
		}

		*desc = ntohll(handle);
	}

	return (ans.ret == 0 ? 0 : -ans.ret_errno);
}
