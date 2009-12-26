/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "buffer.h"
#include "command.h"
#include "config.h"
#include "defines.h"
#include "instance_server.h"
#include "resume/cleanup.h"
#include "sendrecv_server.h"
#include "server.h"
#include "server_handlers_utils.h"

int _handle_truncate(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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
	
	uint32_t offset = (uint32_t)-1;
	const char *path = 
	unpack_32(&offset, buffer);
	
	if (sizeof(offset)
	+ strlen(path) + 1 != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	errno = 0;
	int result = truncate(path, offset);
	
	struct answer ans = { cmd_truncate, 0, result, errno };
	
	free_buffer(buffer);
	
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return result == 0 ? 0 : 1;
}

int _handle_unlink(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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
	int result = unlink(path);
	
	struct answer ans = { cmd_unlink, 0, result, errno };
	
	free_buffer(buffer);
	
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return result != 0 ? 1 : 0;
}

int _handle_rename(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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
	
	uint32_t len = 0;
	const char *path = 
	unpack_32(&len, buffer);
	
	const char *new_path = path + len;
	
	if (sizeof(len)
	+ strlen(path) + 1
	+ strlen(new_path) + 1 != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	errno = 0;
	int result = rename(path, new_path);
	
	struct answer ans = { cmd_rename, 0, result, errno };
	
	free_buffer(buffer);
	
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return result != 0 ? 1 : 0;
}

int _handle_mknod(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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
	const char *path = 
	unpack_32(&mode, buffer);
	
	if (sizeof(mode)
	+ strlen(path) + 1 != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	errno = 0;
	int ret = creat(path, mode & 0777);
	if ( ret != -1 )
	{
		close(ret);
	}
	
	struct answer ans = { cmd_mknod, 0, ret, errno };
	
	free_buffer(buffer);
	
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return 0;
}

int _handle_create(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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
	uint16_t rfs_flags = 0;

	const char *path = 
	unpack_16(&rfs_flags, 
	unpack_32(&mode, buffer
	));
	
	if (sizeof(mode) 
	+ sizeof(rfs_flags) 
	+ strlen(path) + 1 != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}

	int flags = os_file_flags(rfs_flags);
	uint64_t handle = (uint64_t)(-1);

	errno = 0;
	int fd = open(path, flags, mode & 0777);

	struct answer ans = { cmd_create, (fd == -1 ? 0 : sizeof(handle)), (fd == -1 ? -1 : 0), errno };
	
	free_buffer(buffer);
	
	if (fd != -1)
	{
		if (cleanup_add_file_to_open_list(&instance->cleanup.open_files, fd) != 0)
		{
			close(fd);
			return reject_request(instance, cmd, ECANCELED) == 0 ? 1 : -1;
		}
	}

	if (ans.ret == -1)
	{
		if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
		{
			return -1;
		}
	}
	else
	{
		handle = htonll((uint64_t)fd);
		
		send_token_t token = { 0, {{ 0 }} };
		if (do_send(&instance->sendrecv, 
			queue_data((void *)&handle, sizeof(handle), 
			queue_ans(&ans, &token))) < 0)
		{
			return -1;
		}
	}

	return 0;
}

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

