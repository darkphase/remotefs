/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
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
#include "sendrecv.h"

int _rfs_readdir(struct rfs_instance *instance, const char *path, const rfs_readdir_callback_t callback, void *callback_data)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path) + 1;

	struct command cmd = { cmd_readdir, path_len };

	if (rfs_send_cmd_data(&instance->sendrecv, &cmd, path) == -1)
	{
		return -ECONNABORTED;
	}

	struct answer ans = { 0 };
	struct stat stbuf = { 0 };
	uint16_t stat_failed = 0;

	char full_path[FILENAME_MAX + 1] = { 0 };
	unsigned buffer_size = stat_size() + sizeof(full_path);
	char *buffer = get_buffer(buffer_size);
	
	char operation_failed = 0;
	do
	{
		if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
		{
			if (buffer != NULL)
			{
				free_buffer(buffer);
			}
			
			return -ECONNABORTED;
		}
		
		if (ans.command != cmd_readdir)
		{
			if (buffer != NULL)
			{
				free_buffer(buffer);
			}
			
			return cleanup_badmsg(instance, &ans);
		}
		
		if (ans.ret == -1)
		{
			if (buffer != NULL)
			{
				free_buffer(buffer);
			}
			
			return -ans.ret_errno;
		}
		
		if (ans.data_len == 0)
		{
			break;
		}
		
		if (ans.data_len > buffer_size)
		{
			free_buffer(buffer);
		}
		
		memset(buffer, 0, buffer_size);
		
		if (rfs_receive_data(&instance->sendrecv, buffer, ans.data_len) == -1)
		{
			free_buffer(buffer);
			
			return -ECONNABORTED;
		}
		
#ifdef RFS_DEBUG
		dump(buffer, ans.data_len);
#endif
		
		int stat_ret = 0;
		off_t last_pos = unpack_stat(instance, buffer, &stbuf, &stat_ret);
		
		if (stat_ret != 0)
		{
			free_buffer(buffer);
			return -stat_ret;
		}
			
		last_pos = unpack_16(&stat_failed, buffer, last_pos);

		char *entry_name = buffer + last_pos;

		if (stat_failed == 0)
		{
			int joined = path_join(full_path, sizeof(full_path), path, entry_name);
			
			if (joined < 0)
			{
				operation_failed = 1;
				return -EINVAL;
			}
			
			if (joined == 0)
			{
				cache_file(instance, full_path, &stbuf); /* ignore result because cache may be full */
			}
		}
		
		if (operation_failed == 0)
		{
			if (callback(entry_name, callback_data) != 0)
			{
				break;
			}
		}
	}
	while (ans.data_len > 0);

	if (buffer != NULL)
	{
		free_buffer(buffer);
	}

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

	char *buffer = get_buffer(cmd.data_len);

	pack(path, path_len, buffer, 
	pack_32(&fmode, buffer, 0
	));

	if (rfs_send_cmd_data(&instance->sendrecv, &cmd, buffer) == -1)
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

	if (ans.command != cmd_mkdir 
	|| ans.data_len != 0)
	{
		return cleanup_badmsg(instance, &ans);
	}

	if (ans.ret == 0)
	{
		delete_from_cache(instance, path);
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

	if (rfs_send_cmd_data(&instance->sendrecv, &cmd, path) == -1)
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
		delete_from_cache(instance, path);
	}

	return ans.ret == -1 ? -ans.ret_errno : ans.ret;
}

