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
#include "../path.h"
#include "../sendrecv_client.h"
#include "readdir.h"
#include "utils.h"

int _rfs_readdir(struct rfs_instance *instance, const char *path, const rfs_readdir_callback_t callback, void *callback_data)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path) + 1;

	struct command cmd = { cmd_readdir, path_len };

	send_token_t token = { 0 };
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
