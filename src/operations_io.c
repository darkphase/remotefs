/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "attr_cache.h"
#include "buffer.h"
#include "command.h"
#include "compat.h"
#include "config.h"
#include "data_cache.h"
#include "instance_client.h"
#include "operations.h"
#include "operations_rfs.h"
#include "operations_write.h"
#include "operations_utils.h"
#include "path.h"
#include "resume/resume.h"
#include "sendrecv_client.h"

int _rfs_open(struct rfs_instance *instance, const char *path, int flags, uint64_t *desc)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path) + 1;
	uint16_t fi_flags = rfs_file_flags(flags);
	
	unsigned overall_size = sizeof(fi_flags) + path_len;

	struct command cmd = { cmd_open, overall_size };

	char *buffer = get_buffer(cmd.data_len);

	pack(path, path_len, 
	pack_16(&fi_flags, buffer
	));

	send_token_t token = { 0, {{ 0 }} };
	if (do_send(&instance->sendrecv, 
		queue_data(buffer, overall_size, 
		queue_cmd(&cmd, &token))) < 0)
	{
		free_buffer(buffer);
		return -ECONNABORTED;
	}

	free_buffer(buffer);

	struct answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}

	if (ans.command != cmd_open)
	{
		return cleanup_badmsg(instance, &ans);
	}

	if (ans.ret != -1)
	{
		uint64_t handle = (uint64_t)-1;
		
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

	if (ans.ret == -1)
	{
		if (ans.ret_errno == -ENOENT)
		{
			delete_from_cache(instance, path);
		}
	}
	else
	{
		delete_from_cache(instance, path);
		resume_add_file_to_open_list(&instance->resume.open_files, path, flags, *desc);
	}

	return ans.ret == -1 ? -ans.ret_errno : ans.ret;
}

int _rfs_release(struct rfs_instance *instance, const char *path, uint64_t desc)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}
	
	int flush_ret = flush_write(instance, path, desc); /* make sure no data is buffered */
	if (flush_ret < 0)
	{
		return flush_ret;
	}

	clear_cache_by_desc(&instance->write_cache.cache, desc);
	
	uint64_t handle = htonll(desc);

	struct command cmd = { cmd_release, sizeof(handle) };

	send_token_t token = { 0, {{ 0 }} };
	if (do_send(&instance->sendrecv, 
		queue_data((void *)&handle, sizeof(handle), 
		queue_cmd(&cmd, &token))) < 0)
	{
		return -ECONNABORTED;
	}

	struct answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == 0)
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
		delete_from_cache(instance, path);
		resume_remove_file_from_open_list(&instance->resume.open_files, path);
	}

	return ans.ret == -1 ? -ans.ret_errno : ans.ret;
}

