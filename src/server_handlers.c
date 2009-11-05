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
#include <sys/types.h>
#include <string.h>
#include <utime.h>

#include "buffer.h"
#include "command.h"
#include "config.h"
#include "cleanup.h"
#include "exports.h"
#include "id_lookup.h"
#include "instance_server.h"
#include "sendrecv_server.h"
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

	uint32_t user_len = 0, user_len_hton = 0;
	uint32_t group_len = 0, group_len_hton = 0;
	char stat_buffer[STAT_BLOCK_SIZE] = { 0 };
	const char *user = NULL, *group = NULL;

#ifdef WITH_UGO
	if ((instance->server.mounted_export->options & OPT_UGO) != 0)
	{
		user = get_uid_name(instance->id_lookup.uids, stbuf.st_uid);
		group = get_gid_name(instance->id_lookup.gids, stbuf.st_gid);
	}
#endif

	if (user == NULL) { user = ""; }
	if (group == NULL) { group = ""; }
	
	user_len = user_len_hton = strlen(user) + 1;
	group_len = group_len_hton = strlen(group) + 1;
	
	unsigned overall_size = STAT_BLOCK_SIZE + sizeof(user_len) + sizeof(group_len) + user_len + group_len;
	struct answer ans = { cmd_getattr, overall_size, 0, 0 };

	DEBUG("mode: %u, size: %lu\n", (unsigned)stbuf.st_mode, (long unsigned)stbuf.st_size);

	pack_stat(&stbuf, stat_buffer);

	DEBUG("user: %s, user_len: %lu, group: %s, group_len: %lu\n", 
		user, (unsigned long)user_len, 
		group, (unsigned long)group_len);
	
	send_token_t token = { 0, {{ 0 }} };
	return (do_send(&instance->sendrecv, 
		queue_data(group, group_len, 
		queue_data(user, user_len, 
		queue_32(&group_len_hton, 
		queue_32(&user_len_hton, 
		queue_data((const char *)stat_buffer, sizeof(stat_buffer), 
		queue_ans(&ans, &token 
		))))))) < 0) ? -1 : 0;
}

static int process_utime(struct rfsd_instance *instance, const struct command *cmd, const char *path, unsigned is_null, uint64_t actime, uint64_t modtime)
{
	struct utimbuf *buf = NULL;
	
	if (is_null == 0)
	{
		buf = get_buffer(sizeof(*buf));
		buf->modtime = modtime;
		buf->actime = actime;
	}
	
	errno = 0;
	int result = utime(path, buf);
	
	struct answer ans = { cmd->command, 0, result, errno };
	
	if (buf != NULL)
	{
		free_buffer(buf);
	}
		
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return (result == 0 ? 0 : 1);
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

	const char *path = 
	unpack_64(&actime, 
	unpack_64(&modtime, 
	unpack_16(&is_null, buffer
	)));
	
	if (sizeof(actime)
	+ sizeof(modtime)
	+ sizeof(is_null)
	+ strlen(path) + 1 != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}

	int ret = process_utime(instance, cmd, path, is_null, actime, modtime);

	free_buffer(buffer);

	return ret;
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

	const char *path = 
	unpack_16(&is_null, 
	unpack_64(&actime_nsec, 
	unpack_64(&actime_sec, 
	unpack_64(&modtime_nsec, 
	unpack_64(&modtime_sec, buffer
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
	
	int ret = process_utime(instance, cmd, path, is_null, actime_sec, modtime_sec);

	free_buffer(buffer);
	
	return ret;
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
	
	pack_32(&namemax, 
	pack_32(&ffree, 
	pack_32(&files, 
	pack_32(&bavail, 
	pack_32(&bfree, 
	pack_32(&blocks, 
	pack_32(&bsize, buffer
	)))))));

	send_token_t token = { 0, {{ 0 }} };
	if (do_send(&instance->sendrecv, 
		queue_data(buffer, overall_size, 
		queue_ans(&ans, &token))) < 0)
	{
		free_buffer(buffer);
		return -1;
	}

	free_buffer(buffer);

	return 0;
}

