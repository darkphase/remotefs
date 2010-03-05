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
#include "../sendrecv_client.h"
#include "operations_rfs.h"

int _rfs_chmod(struct rfs_instance *instance, const char *path, mode_t mode)
{
#ifndef WITH_UGO
	/* actually dummy to keep some software happy. 
	do not replace with -EACCES or something */
	return 0; 
#else	
	if ((instance->client.export_opts & OPT_UGO) == 0)
	{
		/* do nothing, since this is not UGOed export */
		return 0; 
	}

	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path) + 1;
	uint32_t fmode = mode;

	unsigned overall_size = sizeof(fmode) + path_len;

	struct command cmd = { cmd_chmod, overall_size };

	char *buffer = malloc(overall_size);
	
	pack(path, path_len, 
	pack_32(&fmode, buffer
	));

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

	if (ans.command != cmd_chmod)
	{
		return cleanup_badmsg(instance, &ans);
	}

	if ( ans.ret_errno == 0 )
	{
		delete_from_cache(&instance->attr_cache, path);
	}

	return ans.ret == 0 ? 0 : -ans.ret_errno;
#endif
}
