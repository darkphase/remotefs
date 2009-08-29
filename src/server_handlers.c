/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <string.h>
#include <utime.h>

#include "buffer.h"
#include "command.h"
#include "config.h"
#include "cleanup.h"
#include "exports.h"
#include "instance_server.h"
#include "sendrecv.h"
#include "server.h"
#include "server_handlers_utils.h"

int _handle_getattr(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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
	
	struct stat stbuf = { 0 };
	const char *path = buffer;
	
	if (strlen(path) + 1 != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	errno = stat_file(instance, path, &stbuf);
	if (errno != 0)
	{
		int saved_errno = errno;
		
		free_buffer(buffer);
		return reject_request(instance, cmd, saved_errno) == 0 ? 1 : -1;
	}
	
	free_buffer(buffer);
	
	int size_ret = 0;
	unsigned overall_size = stat_size(instance, &stbuf, &size_ret);

	if (size_ret != 0)
	{
		return reject_request(instance, cmd, size_ret) == 0 ? 1: -1;
	}
	
	struct answer ans = { cmd_getattr, overall_size, 0, 0 };

	buffer = get_buffer(ans.data_len);

	int pack_ret = 0;
	pack_stat(instance, buffer, &stbuf, &pack_ret);

	if (pack_ret != 0)
	{
		return reject_request(instance, cmd, pack_ret) == 0 ? 1: -1;
	}
	
	if (rfs_send_answer_data(&instance->sendrecv, &ans, buffer) == -1)
	{
		free_buffer(buffer);
		return -1;
	}

	free_buffer(buffer);
	
	return 0;
}

int _handle_utime(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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
	
	uint16_t is_null = 0;
	uint64_t modtime = 0;
	uint64_t actime = 0;

	const char *path = buffer + 
	unpack_64(&actime, buffer, 
	unpack_64(&modtime, buffer, 
	unpack_16(&is_null, buffer, 0
	)));
	
	if (sizeof(actime)
	+ sizeof(modtime)
	+ sizeof(is_null)
	+ strlen(path) + 1 != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	struct utimbuf *buf = NULL;
	
	if (is_null == 0)
	{
		buf = get_buffer(sizeof(*buf));
		buf->modtime = modtime;
		buf->actime = actime;
	}
	
	errno = 0;
	int result = utime(path, buf);
	
	struct answer ans = { cmd_utime, 0, result, errno };
	
	if (buf != NULL)
	{
		free_buffer(buf);
	}
	
	free_buffer(buffer);
	
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return (result == 0 ? 0 : 1);
}

int _handle_utimens(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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
	
	uint16_t is_null = 0;
	uint64_t modtime_sec = 0;
	uint64_t modtime_nsec = 0;
	uint64_t actime_sec = 0;
	uint64_t actime_nsec = 0;

	const char *path = buffer +  
	unpack_16(&is_null, buffer, 
	unpack_64(&actime_nsec, buffer, 
	unpack_64(&actime_sec, buffer, 
	unpack_64(&modtime_nsec, buffer, 
	unpack_64(&modtime_sec, buffer, 0
	)))));

	if (sizeof(actime_sec)
	+ sizeof(actime_nsec)
	+ sizeof(modtime_sec)
	+ sizeof(modtime_nsec)
	+ sizeof(is_null)
	+ strlen(path) + 1 != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	DEBUG("is_null: %u\n", (unsigned)is_null);
	
	struct timeval *tv = NULL;
	
	if (is_null == 0)
	{
		tv = get_buffer(sizeof(*tv) * 2);
		tv[0].tv_sec  = (long)(actime_sec);
		tv[0].tv_usec = (long)(actime_nsec / 1000);
		tv[1].tv_sec  = (long)(modtime_sec);
		tv[1].tv_usec = (long)(modtime_nsec / 1000);
	}
	
	errno = 0;
	int result = utimes(path, tv);
	
	struct answer ans = { cmd_utimens, 0, result, errno };
	
	if (tv != NULL)
	{
		free_buffer(tv);
	}
	
	free_buffer(buffer);
	
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return (result == 0 ? 0 : 1);
}

int _handle_statfs(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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
	
	struct statvfs buf = { 0 };
	const char *path = buffer;
	
	if (strlen(path) + 1 != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	errno = 0;
	int result = statvfs(path, &buf);
	int saved_errno = errno;
	
	free_buffer(buffer);
	
	if (result != 0)
	{
		struct answer ans = { cmd_statfs, 0, -1, saved_errno };
		
		if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
		{
			return -1;
		}
		
		return 1;
	}
	
	uint32_t bsize = buf.f_bsize;
	uint32_t blocks = buf.f_blocks;
	uint32_t bfree = buf.f_bfree;
	uint32_t bavail = buf.f_bavail;
	uint32_t files = buf.f_files;
	uint32_t ffree = buf.f_bfree;
	uint32_t namemax = buf.f_namemax;
	
	unsigned overall_size = sizeof(bsize)
	+ sizeof(blocks)
	+ sizeof(bfree)
	+ sizeof(bavail)
	+ sizeof(files)
	+ sizeof(ffree)
	+ sizeof(namemax);
	
	struct answer ans = { cmd_statfs, overall_size, 0, 0 };

	buffer = get_buffer(ans.data_len);
	if (buffer == NULL)
	{
		return reject_request(instance, cmd, ECANCELED) == 0 ? 1 : -1;
	}
	
	pack_32(&namemax, buffer, 
	pack_32(&ffree, buffer, 
	pack_32(&files, buffer, 
	pack_32(&bavail, buffer, 
	pack_32(&bfree, buffer, 
	pack_32(&blocks, buffer, 
	pack_32(&bsize, buffer, 0
		)))))));

	if (rfs_send_answer_data(&instance->sendrecv, &ans, buffer) == -1)
	{
		free_buffer(buffer);
		return -1;
	}

	free_buffer(buffer);

	return 0;
}

