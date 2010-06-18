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
#include <sys/stat.h>

#include "../buffer.h"
#include "../command.h"
#include "../config.h"
#include "../exports.h"
#include "../handling.h"
#include "../id_lookup.h"
#include "../instance_server.h"
#include "../resume/cleanup.h"
#include "../sendrecv_server.h"
#include "utils.h"

int _handle_open(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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
	
	uint16_t rfs_flags = 0;
	const char *path = 
	unpack_16(&rfs_flags, buffer);
	
	if (sizeof(rfs_flags) 
	+ strlen(path) + 1 != cmd->data_len)
	{
		free(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	int flags = os_file_flags(rfs_flags);
	
	errno = 0;
	int fd = open(path, flags);
		
	free(buffer);
	
	if (fd != -1)
	{
		if (cleanup_add_file_to_open_list(&instance->cleanup.open_files, fd) != 0)
		{
			close(fd);
			return reject_request(instance, cmd, ECANCELED) == 0 ? 1 : -1;
		}
	}

	if (fd == -1)
	{
		struct answer ans = { cmd_open, 0, -1, errno };
		if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
		{
			return -1;
		}

		return 1;
	}

	uint64_t handle = (uint64_t)(fd);

	uint32_t stat_failed = 0;
	struct stat stbuf = { 0 };

	stat_failed = (fstat(fd, &stbuf) == 0 ? 0 : 1);

	uint32_t user_len = 0;
	uint32_t group_len = 0;
	char stat_buffer[STAT_BLOCK_SIZE] = { 0 };
	const char *user = NULL, *group = NULL;

#ifdef WITH_UGO
	if (stat_failed == 0 
	&& (instance->server.mounted_export->options & OPT_UGO) != 0)
	{
		user = get_uid_name(instance->id_lookup.uids, stbuf.st_uid);
		group = get_gid_name(instance->id_lookup.gids, stbuf.st_gid);
	}
#endif

	if (user == NULL) { user = ""; }
	if (group == NULL) { group = ""; }
	
	user_len = strlen(user) + 1;
	group_len = strlen(group) + 1;
	
	unsigned overall_size = sizeof(handle) + sizeof(stat_failed) + STAT_BLOCK_SIZE + sizeof(user_len) + sizeof(group_len) + user_len + group_len;

	struct answer ans = { cmd_open, overall_size, 0, 0 };

	DEBUG("mode: %u, size: %lu\n", (unsigned)stbuf.st_mode, (long unsigned)stbuf.st_size);

	pack_stat(&stbuf, stat_buffer);

	DEBUG("user: %s, user_len: %lu, group: %s, group_len: %lu\n", 
		user, (unsigned long)user_len, 
		group, (unsigned long)group_len);
	
	send_token_t token = { 0 };
	return (do_send(&instance->sendrecv, 
		queue_data(group, group_len, 
		queue_data(user, user_len, 
		queue_32(&group_len, 
		queue_32(&user_len, 
		queue_data((const char *)stat_buffer, sizeof(stat_buffer), 
		queue_32(&stat_failed, 
		queue_64(&handle, 
		queue_ans(&ans, &token 
		))))))))) < 0) ? -1 : 0;
}
