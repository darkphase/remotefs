/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <utime.h>

#include "attr_cache.h"
#include "buffer.h"
#include "command.h"
#include "compat.h"
#include "config.h"
#include "data_cache.h"
#include "inet.h"
#include "instance_client.h"
#include "list.h"
#include "operations.h"
#include "operations_rfs.h"
#include "operations_write.h"
#include "operations_utils.h"
#include "path.h"
#include "resume.h"
#include "sendrecv_client.h"

int _rfs_getattr(struct rfs_instance *instance, const char *path, struct stat *stbuf)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	const struct tree_item *cached_value = get_cache(instance, path);
	if (cached_value != NULL 
	&& (time(NULL) - cached_value->time) < ATTR_CACHE_TTL )
	{
		DEBUG("%s is cached\n", path);
		memcpy(stbuf, &cached_value->data, sizeof(*stbuf));
		return 0;
	}

	unsigned path_len = strlen(path) + 1;

	struct command cmd = { cmd_getattr, path_len };
	if (commit_send(&instance->sendrecv, 
		queue_data(path, path_len, 
		queue_cmd(&cmd, send_token(2)))) < 0)
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

	DEBUG("mode: %u, size: %lu\n", stbuf->st_mode, stbuf->st_size);

	stbuf->st_uid = resolve_username(instance, user);
	stbuf->st_gid = resolve_groupname(instance, group, user);

	if (ans.ret_errno == 0)
	{
		cache_file(instance, path, stbuf); /* ignore result because cache may be already full */
	}

	DEBUG("user: %s, group: %s, ret: %d, errno: %u\n", user, group, (int)ans.ret, (unsigned)ans.ret_errno);

	return (ans.ret == -1 ? -ans.ret_errno : ans.ret);
}

static inline int flush_for_utime(struct rfs_instance *instance, const char *path)
{
	/* yes, it's strange indeed, that file is flushing during utime and utimens calls
	however, remotefs may cache write operations until release call
	so if utime[ns] is called before release() (just like for `cp -p`)
	then folowing release() and included flush() will invalidate modification time and etc

	so flushing is here */

	uint64_t desc = is_file_in_open_list(instance, path);

	if (desc != (uint64_t)-1)
	{
		return flush_write(instance, path, desc);
	}

	return 0;
}

int _rfs_utime(struct rfs_instance *instance, const char *path, struct utimbuf *buf)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	if (flush_for_utime(instance, path) < 0)
	{
		return -ECANCELED;
	}

	unsigned path_len = strlen(path) + 1;
	uint64_t actime = 0;
	uint64_t modtime = 0;
	uint16_t is_null = (buf == NULL ? 1 : 0);

	if (buf != 0)
	{
		actime = buf->actime;
		modtime = buf->modtime;
	}

	unsigned overall_size = path_len + sizeof(actime) + sizeof(modtime) + sizeof(is_null);

	struct command cmd = { cmd_utime, overall_size };

	char *buffer = get_buffer(cmd.data_len);

	pack(path, path_len, 
	pack_64(&actime, 
	pack_64(&modtime, 
	pack_16(&is_null, buffer
	))));

	if (commit_send(&instance->sendrecv, 
		queue_data(buffer, overall_size, 
		queue_cmd(&cmd, send_token(2)))) < 0)
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

	if (ans.command != cmd_utime 
	|| ans.data_len != 0)
	{
		return cleanup_badmsg(instance, &ans);
	}

	if (ans.ret == 0)
	{
		delete_from_cache(instance, path);
	}

	return (ans.ret == -1 ? -ans.ret_errno : ans.ret);
}

int _rfs_utimens(struct rfs_instance *instance, const char *path, const struct timespec tv[2])
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	if (flush_for_utime(instance, path) < 0)
	{
		return -ECANCELED;
	}
	
	unsigned path_len = strlen(path) + 1;
	uint64_t actime_sec = 0;
	uint64_t actime_nsec = 0;
	uint64_t modtime_sec = 0;
	uint64_t modtime_nsec = 0;
	uint16_t is_null = (tv == NULL ? 1 : 0);

	if (tv != NULL)
	{
		actime_sec   = (uint64_t)tv[0].tv_sec;
		actime_nsec  = (uint64_t)tv[0].tv_nsec;
		modtime_sec  = (uint64_t)tv[1].tv_sec;
		modtime_nsec = (uint64_t)tv[1].tv_nsec;
	}

	unsigned overall_size = path_len 
		+ sizeof(actime_sec) 
		+ sizeof(actime_nsec) 
		+ sizeof(modtime_sec) 
		+ sizeof(modtime_nsec)
		+ sizeof(is_null);

	struct command cmd = { cmd_utimens, overall_size };

	char *buffer = get_buffer(cmd.data_len);

	pack(path, path_len, 
	pack_16(&is_null, 
	pack_64(&actime_nsec, 
	pack_64(&actime_sec, 
	pack_64(&modtime_nsec, 
	pack_64(&modtime_sec, buffer
	))))));

	if (commit_send(&instance->sendrecv, 
		queue_data(buffer, overall_size, 
		queue_cmd(&cmd, send_token(2)))) < 0)
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

	if (ans.command != cmd_utimens 
	|| ans.data_len != 0)
	{
		return cleanup_badmsg(instance, &ans);
	}

	if (ans.ret == 0)
	{
		delete_from_cache(instance, path);
	}

	return (ans.ret == -1 ? -ans.ret_errno : ans.ret);
}

int _rfs_statfs(struct rfs_instance *instance, const char *path, struct statvfs *buf)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path) + 1;

	struct command cmd = { cmd_statfs, path_len };

	if (commit_send(&instance->sendrecv, 
		queue_data(path, path_len, 
		queue_cmd(&cmd, send_token(2)))) < 0)
	{
		return -ECONNABORTED;
	}

	struct answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}

	if (ans.ret == -1)
	{
		return -ans.ret_errno;
	}

	uint32_t bsize = 0;
	uint32_t blocks = 0;
	uint32_t bfree = 0;
	uint32_t bavail = 0;
	uint32_t files = 0;
	uint32_t ffree = 0;
	uint32_t namemax = 0;

	unsigned overall_size = sizeof(bsize)
	+ sizeof(blocks)
	+ sizeof(bfree)
	+ sizeof(bavail)
	+ sizeof(files)
	+ sizeof(ffree)
	+ sizeof(namemax);

	if (ans.command != cmd_statfs 
	|| ans.data_len != overall_size)
	{
		return cleanup_badmsg(instance, &ans);
	}

	char *buffer = get_buffer(ans.data_len);

	if (rfs_receive_data(&instance->sendrecv, buffer, ans.data_len) == -1)
	{
		free_buffer(buffer);
		return -ECONNABORTED;
	}

	unpack_32(&namemax, 
	unpack_32(&ffree, 
	unpack_32(&files, 
	unpack_32(&bavail, 
	unpack_32(&bfree, 
	unpack_32(&blocks, 
	unpack_32(&bsize, buffer
	)))))));
	
	free_buffer(buffer);

	buf->f_bsize = bsize;
	buf->f_blocks = blocks;
	buf->f_bfree = bfree;
	buf->f_bavail = bavail;
	buf->f_files = files;
	buf->f_ffree = ffree;
	buf->f_namemax = namemax;

	return ans.ret;
}

