/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>

#include "attr_cache.h"
#include "buffer.h"
#include "command.h"
#include "compat.h"
#include "config.h"
#include "data_cache.h"
#include "instance_client.h"
#include "operations.h"
#include "operations_rfs.h"
#include "operations_utils.h"
#include "path.h"
#include "sendrecv_client.h"

int _rfs_readdir(struct rfs_instance *instance, const char *path, const rfs_readdir_callback_t callback, void *callback_data)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path) + 1;

	struct command cmd = { cmd_readdir, path_len };

	send_token_t token = { 0, {{ 0 }} };
	if (do_send(&instance->sendrecv, 
		queue_data(path, path_len, 
		queue_cmd(&cmd, &token))) < 0)
	{
		return -ECONNABORTED;
	}

	struct answer ans = { 0 };
	struct stat stbuf = { 0 };
	uint16_t stat_failed = 0;

	uint32_t user_len = 0;
	uint32_t group_len = 0;
	uint32_t entry_len = 0;

	char full_path[FILENAME_MAX + 1] = { 0 };

#define buffer_size sizeof(stat_failed) + STAT_BLOCK_SIZE + sizeof(user_len) + sizeof(group_len) + sizeof(entry_len) \
+ sizeof(full_path) + (MAX_SUPPORTED_NAME_LEN + 1) /*user */ + (MAX_SUPPORTED_NAME_LEN + 1) /* group */
	
	char buffer[buffer_size] = { 0 };

#undef buffer_size

	do
	{
		if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
		{
			return -ECONNABORTED;
		}
		
		if (ans.command != cmd_readdir)
		{
			return cleanup_badmsg(instance, &ans);
		}
		
		if (ans.ret == -1)
		{
			return -ans.ret_errno;
		}
		
		if (ans.data_len == 0)
		{
			break;
		}
		
		if (ans.data_len > sizeof(buffer))
		{
			return -EINVAL;
		}
		
		memset(buffer, 0, sizeof(buffer));
		
		if (rfs_receive_data(&instance->sendrecv, buffer, ans.data_len) == -1)
		{
			return -ECONNABORTED;
		}
		
		const char *user = 
		unpack_32(&entry_len, 
		unpack_32(&group_len, 
		unpack_32(&user_len, 
		unpack_stat(&stbuf, 
		unpack_16(&stat_failed, buffer
		)))));
		const char *group = user + user_len;
		const char *entry = group + group_len;

		DEBUG("user: %s, group: %s\n", user, group);
		
		if (stat_failed == 0)
		{
			stbuf.st_uid = resolve_username(instance, user);
			stbuf.st_gid = resolve_groupname(instance, group, user);

			int joined = path_join(full_path, sizeof(full_path), path, entry);
			
			if (joined < 0)
			{
				return -EINVAL;
			}
			
			if (joined >= 0)
			{
				cache_file(&instance->attr_cache, full_path, &stbuf); /* ignore result because cache may be full */
			
				if (callback(entry, callback_data) != 0)
				{
					break;
				}
			}
		}	
	}
	while (ans.data_len > 0);

	return 0;
}

int _rfs_mkdir(struct rfs_instance *instance, const char *path, mode_t mode)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path) + 1;

	uint32_t fmode = mode;

	unsigned overall_size = sizeof(fmode) + path_len;

	struct command cmd = { cmd_mkdir, overall_size };

	char *buffer = malloc(cmd.data_len);

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

	if (ans.command != cmd_mkdir 
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

