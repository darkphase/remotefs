/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

#include "buffer.h"
#include "command.h"
#include "config.h"
#include "instance_server.h"
#include "path.h"
#include "sendrecv.h"
#include "server.h"
#include "server_handlers_utils.h"

int _handle_readdir(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(&instance->sendrecv, buffer, cmd->data_len) == -1)
	{
		free_buffer(buffer);
		return -1;
	}

	char *path = buffer;
	unsigned path_len = strlen(path) + 1;
	
	if (path_len != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	if (path_len > FILENAME_MAX)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	errno = 0;
	DIR *dir = opendir(path);
	
	if (dir == NULL)
	{
		int saved_errno = errno;
		
		free_buffer(path);
		return reject_request(instance, cmd, saved_errno) == 0 ? 1 : -1;
	}

	struct dirent *dir_entry = NULL;
	struct stat stbuf = { 0 };
	uint16_t stat_failed = 0;
	
	char full_path[FILENAME_MAX + 1] = { 0 };
	
	struct answer ans = { cmd_readdir, 0 };
	
	while ((dir_entry = readdir(dir)) != 0)
	{	
		const char *entry_name = dir_entry->d_name;
		unsigned entry_len = strlen(entry_name) + 1;
		
		stat_failed = 0;
		memset(&stbuf, 0, sizeof(stbuf));
		
		int joined = path_join(full_path, sizeof(full_path), path, entry_name);
		if (joined < 0)
		{
			stat_failed = 1;
		}
	
		if (joined == 0)
		{
			if (stat_file(instance, full_path, &stbuf) != 0)
			{
				stat_failed = 1;
			}
		}
		
		int size_ret = 0;
		size_t stat_struct_size = stat_size(instance, &stbuf, &size_ret);

		if (size_ret != 0)
		{
			stat_failed = 1;
		}
		
		unsigned overall_size = stat_struct_size + sizeof(stat_failed) + entry_len;
		buffer = get_buffer(overall_size);

		ans.data_len = overall_size;
		
		int pack_ret = 0; 
		off_t last_offset = pack_stat(instance, buffer, &stbuf, &pack_ret);

		if (pack_ret != 0)
		{
			stat_failed = 1;
		}

		pack(entry_name, entry_len, buffer, 
		pack_16(&stat_failed, buffer, last_offset
		));
		
#ifdef RFS_DEBUG
		dump(buffer, overall_size);
#endif

		if (rfs_send_answer_data(&instance->sendrecv, &ans, buffer) == -1)
		{
			closedir(dir);
			free_buffer(path);
			free_buffer(buffer);
			return -1;
		}

		free_buffer(buffer);
	}

	closedir(dir);
	free_buffer(path);
	
	ans.data_len = 0;
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return 0;
}

int _handle_mkdir(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(&instance->sendrecv, buffer, cmd->data_len) == -1)
	{
		free_buffer(buffer);
		return -1;
	}
	
	uint32_t mode = 0;
	
	const char *path = buffer + 
	unpack_32(&mode, buffer, 0);
	
	if (sizeof(mode)
	+ strlen(path) + 1 != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	errno = 0;
	int result = mkdir(path, mode);
	
	struct answer ans = { cmd_mkdir, 0, result, errno };
	
	free_buffer(buffer);
	
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return result != 0 ? 1 : 0;
}

int _handle_rmdir(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(&instance->sendrecv, buffer, cmd->data_len) == -1)
	{
		free_buffer(buffer);
		return -1;
	}
	
	const char *path = buffer;
	
	if (strlen(path) + 1 != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	errno = 0;
	int result = rmdir(path);
	
	struct answer ans = { cmd_rmdir, 0, result, errno };
	
	free_buffer(buffer);
	
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return result != 0 ? 1 : 0;
}

