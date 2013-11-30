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
#include "../resume/resume.h"
#include "../sendrecv_client.h"
#include "flush.h"
#include "utils.h"

int _rfs_release(struct rfs_instance *instance, const char *path, uint64_t desc)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	int flush_ret = _flush_write(instance, path, desc); /* make sure no data is buffered */
	if (flush_ret < 0)
	{
		return flush_ret;
	}

	uint64_t handle = htonll(desc);

	struct rfs_command cmd = { cmd_release, sizeof(handle) };

	send_token_t token = { 0 };
	if (do_send(&instance->sendrecv,
		queue_data((void *)&handle, sizeof(handle),
		queue_cmd(&cmd, &token))) < 0)
	{
		return -ECONNABORTED;
	}

	struct rfs_answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}

	if (ans.command != cmd_release
	|| ans.data_len != 0)
	{
		return cleanup_badmsg(instance, &ans);
	}

	if (ans.ret == 0)
	{
		delete_from_cache(&instance->attr_cache, path);
		resume_remove_file_from_open_list(&instance->resume.open_files, path);
	}

	return ans.ret == -1 ? -ans.ret_errno : ans.ret;
}
