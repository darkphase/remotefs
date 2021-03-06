/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../buffer.h"
#include "../command.h"
#include "../config.h"
#include "../exports.h"
#include "../handling.h"
#include "../id_lookup.h"
#include "../instance_server.h"
#include "../sendrecv_server.h"
#include "utils.h"

int _handle_chown(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct rfs_command *cmd)
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
	
	uint32_t user_len = 0;
	uint32_t group_len = 0;
	const char *path = 
	unpack_32(&group_len, 
	unpack_32(&user_len, buffer
	));

	unsigned path_len = strlen(path) + 1;
	
	const char *user = path + path_len;
	const char *group = user + user_len;

	if (sizeof(user_len) + sizeof(group_len)
	+ path_len + user_len + group_len != cmd->data_len)
	{
		free(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	uid_t uid = (strlen(user) == 0 ? (uid_t)-1 : get_uid(instance->id_lookup.uids, user));
	gid_t gid = (strlen(group) == 0 ? (gid_t)-1 : get_gid(instance->id_lookup.gids, group));
	
	if (uid == (uid_t)-1
	&& gid == (gid_t)-1) /* if both == -1, then this is error. however one of them == -1 is ok */
	{
		free(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	errno = 0;
	int ret = chown(path, uid, gid);
	
	struct rfs_answer ans = { cmd_chown, 0, ret, errno };
	
	free(buffer);
	
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return 0;
}
