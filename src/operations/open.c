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
#include "utils.h"

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

	char *buffer = malloc(cmd.data_len);

	pack(path, path_len, 
	pack_16(&fi_flags, buffer
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

	if (ans.command != cmd_open)
	{
		return cleanup_badmsg(instance, &ans);
	}

	if (ans.ret == -1)
	{
		if (ans.ret_errno == -ENOENT)
		{
			delete_from_cache(&instance->attr_cache, path);
		}

		return -ans.ret_errno;
	}

	uint32_t stat_failed = 0;
	uint32_t user_len = 0;
	uint32_t group_len = 0;

#define ans_buffer_size sizeof(*desc) \
+ sizeof(stat_failed) + STAT_BLOCK_SIZE + sizeof(user_len) + sizeof(group_len) \
+ (MAX_SUPPORTED_NAME_LEN + 1) + (MAX_SUPPORTED_NAME_LEN + 1)

	char ans_buffer[ans_buffer_size]  = { 0 };
	
	if (ans.data_len > sizeof(ans_buffer))
	{
		return cleanup_badmsg(instance, &ans);
	}
#undef ans_buffer_size
	
	if (rfs_receive_data(&instance->sendrecv, ans_buffer, ans.data_len) == -1)
	{
		return -ECONNABORTED;
	}

	struct stat stbuf = { 0 };

	const char *user = 
	unpack_32(&group_len, 
	unpack_32(&user_len, 
	unpack_stat(&stbuf, 
	unpack_32(&stat_failed, 
	unpack_64(desc, ans_buffer
	)))));
	const char *group = user + user_len;

	DEBUG("handle: %llu\n", (long long unsigned)(*desc));

	stbuf.st_uid = resolve_username(instance, user);
	stbuf.st_gid = resolve_groupname(instance, group, user);

	if (ans.ret_errno == 0)
	{
		if (stat_failed == 0)
		{
			cache_file(&instance->attr_cache, path, &stbuf);
		}
		
		resume_add_file_to_open_list(&instance->resume.open_files, path, flags, *desc);
	}
	else
	{
		delete_from_cache(&instance->attr_cache, path);
	}
	
	return ans.ret == -1 ? -ans.ret_errno : ans.ret;
}
