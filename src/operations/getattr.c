/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "../attr_cache.h"
#include "../buffer.h"
#include "../command.h"
#include "../compat.h"
#include "../config.h"
#include "../instance_client.h"
#include "../sendrecv_client.h"
#include "utils.h"

int _rfs_getattr(struct rfs_instance *instance, const char *path, struct stat *stbuf)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	/* see comments for _rfs_utime 
	and some sofware is getting file attrs without flushing it 
	so if data is still in write cache, this software might get wrong stats */
	if (_flush_file(instance, path) < 0)
	{
		return -ECANCELED;
	}

	const struct tree_item *cached_value = get_cache(&instance->attr_cache, path);
	if (cached_value != NULL 
	&& (time(NULL) - cached_value->time) < ATTR_CACHE_TTL )
	{
		DEBUG("%s is cached\n", path);
		memcpy(stbuf, &cached_value->data, sizeof(*stbuf));
		delete_from_cache(&instance->attr_cache, path); /* destructive reading */
		return 0;
	}

	unsigned path_len = strlen(path) + 1;

	struct command cmd = { cmd_getattr, path_len };
	
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

	if (ans.command != cmd_getattr)
	{
		return cleanup_badmsg(instance, &ans);
	}

	if (ans.ret == -1)
	{
		return -ans.ret_errno;
	}

	uint32_t user_len = 0;
	uint32_t group_len = 0;

#define buffer_size STAT_BLOCK_SIZE + sizeof(user_len) + sizeof(group_len) \
+ (MAX_SUPPORTED_NAME_LEN + 1) + (MAX_SUPPORTED_NAME_LEN + 1)

	char buffer[buffer_size]  = { 0 };

#undef buffer_size

	if (ans.data_len > sizeof(buffer))
	{
		return cleanup_badmsg(instance, &ans);
	}

	if (rfs_receive_data(&instance->sendrecv, buffer, ans.data_len) == -1)
	{
		return -ECONNABORTED;
	}

	const char *user = 
	unpack_32(&group_len, 
	unpack_32(&user_len, 
	unpack_stat(stbuf, buffer
	)));
	const char *group = user + user_len;

	stbuf->st_uid = resolve_username(instance, user);
	stbuf->st_gid = resolve_groupname(instance, group, user);

	if (ans.ret_errno == 0)
	{
		cache_file(&instance->attr_cache, path, stbuf);
	}

	DEBUG("user: %s, group: %s, ret: %d, errno: %u\n", user, group, (int)ans.ret, (unsigned)ans.ret_errno);

	return (ans.ret == -1 ? -ans.ret_errno : ans.ret);
}
