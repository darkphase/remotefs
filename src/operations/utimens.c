/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <utime.h>

#include "../buffer.h"
#include "../command.h"
#include "../compat.h"
#include "../config.h"
#include "../instance_client.h"
#include "../sendrecv_client.h"
#include "utils.h"

int _rfs_utimens(struct rfs_instance *instance, const char *path, const struct timespec tv[2])
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	/* see comments for utime */
	if (_flush_file(instance, path) < 0)
	{
		return -ECANCELED;
	}
	
	unsigned path_len = strlen(path) + 1;
	uint64_t actime_sec = 0;
	uint64_t actime_nsec = 0;
	uint64_t modtime_sec = 0;
	uint64_t modtime_nsec = 0;
	uint16_t is_null = (tv == NULL ? 1 : 0);

	if (tv != NULL)
	{
		actime_sec   = (uint64_t)tv[0].tv_sec;
		actime_nsec  = (uint64_t)tv[0].tv_nsec;
		modtime_sec  = (uint64_t)tv[1].tv_sec;
		modtime_nsec = (uint64_t)tv[1].tv_nsec;
	}

	unsigned overall_size = path_len 
		+ sizeof(actime_sec) 
		+ sizeof(actime_nsec) 
		+ sizeof(modtime_sec) 
		+ sizeof(modtime_nsec)
		+ sizeof(is_null);

	struct command cmd = { cmd_utimens, overall_size };

	char *buffer = malloc(cmd.data_len);

	pack(path, path_len, 
	pack_16(&is_null, 
	pack_64(&actime_nsec, 
	pack_64(&actime_sec, 
	pack_64(&modtime_nsec, 
	pack_64(&modtime_sec, buffer
	))))));

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

	if (ans.command != cmd_utimens 
	|| ans.data_len != 0)
	{
		return cleanup_badmsg(instance, &ans);
	}

	if (ans.ret == 0)
	{
		delete_from_cache(&instance->attr_cache, path);
	}

	return (ans.ret == -1 ? -ans.ret_errno : ans.ret);
}
