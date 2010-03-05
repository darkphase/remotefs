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

#include "../buffer.h"
#include "../command.h"
#include "../compat.h"
#include "../config.h"
#include "../defines.h"
#include "../instance_client.h"
#include "../resume/resume.h"
#include "../sendrecv_client.h"
#include "utils.h"

static unsigned is_file_fully_locked(const struct flock *fl)
{
	return (fl->l_whence == SEEK_SET 
	&& fl->l_start == 0 
	&& fl->l_len == 0) ? 1 : 0;
}

int _rfs_lock(struct rfs_instance *instance, const char *path, uint64_t desc, int lock_cmd, struct flock *fl)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}
	
	if (fl->l_type == F_UNLCK 
	&& resume_is_file_in_locked_list(instance->resume.locked_files, path) == 0)
	{
		return 0;
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
	if (fl->l_type == F_UNLCK)
	{
		type = RFS_UNLCK;
	}
	else 
	{
		if ((fl->l_type & F_RDLCK) != 0)
		{
			type |= RFS_RDLCK;
		}
		if ((fl->l_type & F_WRLCK) != 0)
		{
			type |= RFS_WRLCK;
		}
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
	
	send_token_t token = { 0, {{ 0 }} };
	if (do_send(&instance->sendrecv, 
		queue_data(buffer, overall_size, 
		queue_cmd(&cmd, &token))) < 0)
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
	
	if (ans.ret == 0 
	&& (lock_cmd == F_SETLK || lock_cmd == F_SETLKW))
	{
		if (fl->l_type == F_UNLCK)
		{
			resume_remove_file_from_locked_list(&instance->resume.locked_files, path); 
		}
		else
		{
			resume_add_file_to_locked_list(&instance->resume.locked_files, path, fl->l_type, is_file_fully_locked(fl)); 
		}
	}
	
	return ans.ret != 0 ? -ans.ret_errno : 0;
}
