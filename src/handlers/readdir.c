/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

#include "../buffer.h"
#include "../command.h"
#include "../config.h"
#include "../exports.h"
#include "../handling.h"
#include "../id_lookup.h"
#include "../instance_server.h"
#include "../path.h"
#include "../sendrecv_server.h"
#include "utils.h"

int _handle_readdir(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = malloc(cmd->data_len);
	
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(&instance->sendrecv, buffer, cmd->data_len) == -1)
	{
		free(buffer);
		return -1;
	}

	const char *path = buffer;
	unsigned path_len = strlen(path) + 1;
	
	if (path_len != cmd->data_len)
	{
		free(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	if (path_len > FILENAME_MAX)
	{
		free(buffer);
		return reject_request(instance, cmd, E2BIG) == 0 ? 1 : -1;
	}
	
	errno = 0;
	DIR *dir = opendir(path);
	
	if (dir == NULL)
	{
		int saved_errno = errno;
		
		free(buffer);
		return reject_request(instance, cmd, saved_errno) == 0 ? 1 : -1;
	}

	struct dirent *dir_entry = NULL;
	struct stat stbuf = { 0 };
	uint16_t stat_failed = 0;
	char stat_buffer[STAT_BLOCK_SIZE] = { 0 };
	char full_path[FILENAME_MAX + 1] = { 0 };
	
	while ((dir_entry = readdir(dir)) != NULL)
	{	
		const char *entry_name = dir_entry->d_name;
		uint32_t entry_len = strlen(entry_name) + 1, entry_len_hton = entry_len;
		
		stat_failed = 0;
		char *send_path = full_path;
		
		int joined = path_join(full_path, sizeof(full_path), path, entry_name);
		if (joined < 0)
		{
			send_path = "???";
			stat_failed = 1;
		}
		else
		{
			send_path = full_path;
		}
	
		if (joined >= 0)
		{
			if (stat_file(instance, full_path, &stbuf) != 0)
			{
				stat_failed = 1;
			}
		}

		const char *user = NULL, *group = NULL;

		if (stat_failed != 0)
		{
			memset(&stbuf, 0, sizeof(stbuf));
		}

#ifdef WITH_UGO		
		if ((instance->server.mounted_export->options & OPT_UGO) != 0)
		{
			user = get_uid_name(instance->id_lookup.uids, stbuf.st_uid);
			group = get_gid_name(instance->id_lookup.gids, stbuf.st_gid);

			if (user != NULL && strlen(user) > MAX_SUPPORTED_NAME_LEN) { user = NULL; }
			if (group != NULL && strlen(group) > MAX_SUPPORTED_NAME_LEN) { group = NULL; }
		}
#endif

		if (user == NULL) { user = ""; }
		if (group == NULL) { group = ""; }

		uint32_t user_len = strlen(user) + 1, user_len_hton = user_len;
		uint32_t group_len = strlen(group) + 1, group_len_hton = group_len;
		
		unsigned overall_size = sizeof(stat_failed) 
			+ STAT_BLOCK_SIZE 
			+ sizeof(user_len_hton) 
			+ sizeof(group_len_hton) 
			+ sizeof(entry_len_hton) 
			+ user_len 
			+ group_len
			+ entry_len;

		pack_stat(&stbuf, stat_buffer);

		struct answer ans = { cmd_readdir, overall_size, 0, 0 };
	
		send_token_t token = { 0, {{ 0 }} };
		if (do_send(&instance->sendrecv, 
			queue_data(entry_name, entry_len, 
			queue_data(group, group_len, 
			queue_data(user, user_len, 
			queue_32(&entry_len_hton, 
			queue_32(&group_len_hton, 
			queue_32(&user_len_hton, 
			queue_data((const char *)stat_buffer, sizeof(stat_buffer), 
			queue_16(&stat_failed, 
			queue_ans(&ans, &token) 
			))))))))) < 0)
		{
			closedir(dir);
			free(buffer);
			return -1;
		}
	}

	closedir(dir);
	free(buffer);

	struct answer last_ans = { cmd_readdir, 0, 0, 0 };
	if (rfs_send_answer(&instance->sendrecv, &last_ans) == -1)
	{
		return -1;
	}
	
	return 0;
}
