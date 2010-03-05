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
#include "operations_rfs.h"
#include "utils.h"

int _rfs_utime(struct rfs_instance *instance, const char *path, struct utimbuf *buf)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	/* yes, it's strange indeed, that file is flushing during utime and utimens calls
	however, remotefs may cache write operations until release call
	so if utime[ns] is called before release() (just like for `cp -p`)
	then folowing release() and included flush() will invalidate modification time and etc

	so flushing is here */
	if (_flush_file(instance, path) < 0)
	{
		return -ECANCELED;
	}

	unsigned path_len = strlen(path) + 1;
	uint64_t actime = 0;
	uint64_t modtime = 0;
	uint16_t is_null = (buf == NULL ? 1 : 0);

	if (buf != 0)
	{
		actime = buf->actime;
		modtime = buf->modtime;
	}

	unsigned overall_size = path_len + sizeof(actime) + sizeof(modtime) + sizeof(is_null);

	struct command cmd = { cmd_utime, overall_size };

	char *buffer = malloc(cmd.data_len);

	pack(path, path_len, 
	pack_64(&actime, 
	pack_64(&modtime, 
	pack_16(&is_null, buffer
	))));

	send_token_t token = { 0, {{ 0 }} };
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

	if (ans.command != cmd_utime 
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
