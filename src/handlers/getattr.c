/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <utime.h>

#include "../buffer.h"
#include "../command.h"
#include "../config.h"
#include "../exports.h"
#include "../handling.h"
#include "../id_lookup.h"
#include "../instance_server.h"
#include "../sendrecv_server.h"
#include "utils.h"

int _handle_getattr(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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
	
	struct stat stbuf = { 0 };
	const char *path = buffer;
	
	if (strlen(path) + 1 != cmd->data_len)
	{
		free(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	errno = stat_file(instance, path, &stbuf);
	if (errno != 0)
	{
		int saved_errno = errno;
		
		free(buffer);
		return reject_request(instance, cmd, saved_errno) == 0 ? 1 : -1;
	}
	
	free(buffer);

	uint32_t user_len = 0;
	uint32_t group_len = 0;
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
	
	user_len = strlen(user) + 1;
	group_len = strlen(group) + 1;
	
	unsigned overall_size = STAT_BLOCK_SIZE + sizeof(user_len) + sizeof(group_len) + user_len + group_len;
	struct answer ans = { cmd_getattr, overall_size, 0, 0 };

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
		queue_ans(&ans, &token 
		))))))) < 0) ? -1 : 0;
}
