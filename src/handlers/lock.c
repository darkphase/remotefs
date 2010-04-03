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
#include <unistd.h>

#include "../buffer.h"
#include "../command.h"
#include "../config.h"
#include "../defines.h"
#include "../handling.h"
#include "../instance_server.h"
#include "../resume/cleanup.h"
#include "../sendrecv_server.h"
#include "utils.h"

int _handle_lock(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	uint16_t flags = 0;
	uint16_t type = 0;
	uint16_t whence = 0;
	uint64_t start = 0;
	uint64_t len = 0;
	uint64_t fd = 0;
	
#define overall_size sizeof(fd) + sizeof(flags) + sizeof(type) + sizeof(whence) + sizeof(start) + sizeof(len)
	if (cmd->data_len != overall_size)
	{
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}

	char buffer[overall_size] = { 0 };
	
	if (rfs_receive_data(&instance->sendrecv, buffer, overall_size) == -1)
	{
		return -1;
	}
#undef overall_size

	unpack_64(&len, 
	unpack_64(&start, 
	unpack_16(&whence, 
	unpack_16(&type, 
	unpack_16(&flags, 
	unpack_64(&fd, buffer
	))))));
	
	int lock_cmd = 0;
	switch (flags)
	{
	case RFS_GETLK:
		lock_cmd = F_GETLK;
		break;
	case RFS_SETLK:
		lock_cmd = F_SETLK;
		break;
	case RFS_SETLKW:
		lock_cmd = F_SETLKW;
		break;
	default:
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	short lock_type = 0;
	switch (type)
	{
	case RFS_UNLCK:
		lock_type = F_UNLCK;
		break;
	case RFS_RDLCK:
		lock_type = F_RDLCK;
		break;
	case RFS_WRLCK:
		lock_type = F_WRLCK;
		break;
	default:
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	DEBUG("lock command: %d (%d)\n", lock_cmd, flags);
	DEBUG("lock type: %d (%d)\n", lock_type, type);
	
	struct flock fl = { 0 };
	fl.l_type = lock_type;
	fl.l_whence = (short)whence;
	fl.l_start = (off_t)start;
	fl.l_len = (off_t)len;
	fl.l_pid = getpid();
	
	errno = 0;
	int ret = fcntl((int)fd, lock_cmd, &fl);
	
	struct answer ans = { cmd_lock, 0, ret, errno };
	
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return 0;
}
