/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include "attr_cache.h"
#include "buffer.h"
#include "command.h"
#include "compat.h"
#include "config.h"
#include "instance_client.h"
#include "operations_rfs.h"
#include "operations_utils.h"
#include "resume.h"
#include "sendrecv_client.h"

int _rfs_truncate(struct rfs_instance *instance, const char *path, off_t offset)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path) + 1;
	uint32_t foffset = offset;

	unsigned overall_size = sizeof(foffset) + path_len;

	struct command cmd = { cmd_truncate, overall_size };

	char *buffer = get_buffer(cmd.data_len);

	pack(path, path_len, 
	pack_32(&foffset, buffer
	));

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

	if (ans.command != cmd_truncate || ans.data_len != 0)
	{
		return cleanup_badmsg(instance, &ans);
	}

	if (ans.ret == 0)
	{
		delete_from_cache(instance, path);
	}

	return ans.ret == -1 ? -ans.ret_errno : ans.ret;
}

int _rfs_unlink(struct rfs_instance *instance, const char *path)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path) + 1;
	struct command cmd = { cmd_unlink, path_len };

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

	if (ans.command != cmd_unlink 
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

int _rfs_rename(struct rfs_instance *instance, const char *path, const char *new_path)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path) + 1;
	unsigned new_path_len = strlen(new_path) + 1;
	uint32_t len = path_len;

	unsigned overall_size = sizeof(len) + path_len + new_path_len;

	struct command cmd = { cmd_rename, overall_size };

	char *buffer = get_buffer(cmd.data_len);

	pack(new_path, new_path_len, 
	pack(path, path_len, 
	pack_32(&len, buffer
	)));

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

	if (ans.command != cmd_rename 
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

int _rfs_mknod(struct rfs_instance *instance, const char *path, mode_t mode, dev_t dev)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path) + 1;
	uint32_t fmode = mode;

	if ((fmode & 0777) == 0)
	{
		fmode |= 0600;
	}

	unsigned overall_size = sizeof(fmode) + path_len;

	struct command cmd = { cmd_mknod, overall_size };

	char *buffer = get_buffer(cmd.data_len);

	pack(path, path_len, 
	pack_32(&fmode, buffer
	));

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

	if (ans.command != cmd_mknod)
	{
		return cleanup_badmsg(instance, &ans);
	}

	if (ans.ret == 0)
	{
		delete_from_cache(instance, path);
	}

	return (ans.ret == 0 ? 0 : -ans.ret_errno);
}

int _rfs_create(struct rfs_instance *instance, const char *path, mode_t mode, int flags, uint64_t *desc)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path) + 1;
	uint32_t fmode = mode;
	uint16_t fi_flags = rfs_file_flags(flags);

	if ((fmode & 0777) == 0)
	{
		fmode |= 0600;
	}

	unsigned overall_size = sizeof(fmode) + sizeof(fi_flags) + path_len;

	struct command cmd = { cmd_create, overall_size };

	char *buffer = get_buffer(cmd.data_len);

	pack(path, path_len, 
	pack_16(&fi_flags, 
	pack_32(&fmode, buffer
	)));

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

	if (ans.command != cmd_create)
	{
		return cleanup_badmsg(instance, &ans);
	}

	if (ans.ret == 0)
	{
		uint64_t handle = (uint64_t)(-1);

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

	return (ans.ret == 0 ? 0 : -ans.ret_errno);
}

int _rfs_lock(struct rfs_instance *instance, const char *path, uint64_t desc, int lock_cmd, struct flock *fl)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}
	
	uint16_t flags = 0;
	
	switch (lock_cmd)
	{
	case F_GETLK:
		flags = RFS_GETLK;
		break;
	case F_SETLK:
		flags = RFS_SETLK;
		break;
	case F_SETLKW:
		flags = RFS_SETLKW;
		break;
	default:
		return -EINVAL;
	}
	
	uint16_t type = 0;
	switch (fl->l_type)
	{
	case F_UNLCK:
		type = RFS_UNLCK;
		break;
	case F_RDLCK:
		type = RFS_RDLCK;
		break;
	case F_WRLCK:
		type = RFS_WRLCK;
		break;
	default:
		return -EINVAL;
	}
	
	uint16_t whence = fl->l_whence;
	uint64_t start = fl->l_start;
	uint64_t len = fl->l_len;
	uint64_t fd = desc;
	
#define overall_size sizeof(fd) + sizeof(flags) + sizeof(type) + sizeof(whence) + sizeof(start) + sizeof(len)
	char buffer[overall_size] = { 0 };
	
	pack_64(&len, 
	pack_64(&start, 
	pack_16(&whence, 
	pack_16(&type, 
	pack_16(&flags, 
	pack_64(&fd, buffer
	))))));
	
	struct command cmd = { cmd_lock, overall_size };
	
	if (commit_send(&instance->sendrecv, 
		queue_data(buffer, overall_size, 
		queue_cmd(&cmd, send_token(2)))) < 0)
	{
		return -ECONNABORTED;
	}
#undef overall_size

	struct answer ans = { 0 };
	
	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}
	
	if (ans.command != cmd_lock 
	|| ans.data_len != 0)
	{
		return cleanup_badmsg(instance, &ans);
	}
	
	if (ans.ret == 0)
	{
		update_file_lock_status(instance, path, lock_cmd, fl); 
	}
	
	return ans.ret != 0 ? -ans.ret_errno : 0;
}

