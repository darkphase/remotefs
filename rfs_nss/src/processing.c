/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <list.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "manage_server.h"
#include "processing.h"
#include "processing_common.h"
#include "nss_cmd.h"

int process_addserver(int sock, const struct nss_command *cmd, struct config *config)
{
	char *rfs_server = NULL;

	if (recv_command_data(sock, cmd, &rfs_server) < 0
	|| rfs_server == NULL
	|| strlen(rfs_server) != cmd->data_len - 1)
	{
		if (rfs_server != NULL)
		{
			free(rfs_server);
		}
		return -1;
	}

	if (fork() != 0)
	{
		free(rfs_server);
		return 0;
	}

	int add_ret = add_rfs_server(config, rfs_server);
		
	free(rfs_server);
		
	if (add_ret < 0)
	{
		reject_request(sock, cmd, -add_ret);
		exit(1);
	}

	struct nss_answer ans = { cmd_addserver, 0, 0, 0 };

	if (send_answer(sock, &ans, NULL) < 0)
	{
		exit(1);
	}

	shutdown(sock, SHUT_RDWR);
	close(sock);

	exit(0);
}

int process_inc(int sock, const struct nss_command *cmd, struct config *config)
{
	++config->connections;
	DEBUG("connections counter: %u\n", config->connections);
	
	config->connections_updated = time(NULL);

	return 0;
}

int process_dec(int sock, const struct nss_command *cmd, struct config *config)
{
	if (config->connections > 0)
	{
		--config->connections;
	}
	DEBUG("connections counter: %u\n", config->connections);

	config->connections_updated = time(NULL);

	return 0;
}

int process_adduser(int sock, const struct nss_command *cmd, struct config *config)
{
	char *buffer = NULL;

	if (recv_command_data(sock, cmd, &buffer) < 0)
	{
		return -1;
	}

	uint32_t user_uid = *((uint32_t *)buffer);
	char *username = buffer + sizeof(user_uid);

	DEBUG("username: %s, uid: %d\n", username, user_uid);
	
	int add_ret = add_user(&config->users, username, (uid_t)user_uid);
	
	free(buffer);

	struct nss_answer ans = { cmd_adduser, 0, (add_ret == 0 ? 0 : -1), -add_ret };
	return send_answer(sock, &ans, NULL);
}

int process_addgroup(int sock, const struct nss_command *cmd, struct config *config)
{
	char *buffer = NULL;

	if (recv_command_data(sock, cmd, &buffer) < 0)
	{
		return -1;
	}

	uint32_t group_gid = *((uint32_t *)buffer);
	char *groupname = buffer + sizeof(group_gid);

	DEBUG("groupname: %s, gid: %d\n", groupname, group_gid);
	
	int add_ret = add_group(&config->groups, groupname, (gid_t)group_gid);
	
	free(buffer);

	struct nss_answer ans = { cmd_addgroup, 0, (add_ret == 0 ? 0 : -1), -add_ret };
	return send_answer(sock, &ans, NULL);
}

int process_getpwnam(int sock, const struct nss_command *cmd, struct config *config)
{
	char *buffer = NULL;

	if (recv_command_data(sock, cmd, &buffer) < 0)
	{
		return -1;
	}

	struct user_info *info = find_user_name(config->users, buffer);

	if (info == NULL)
	{
		return reject_request(sock, cmd, ENOENT);
	}

	struct nss_answer ans = { cmd_getpwnam, sizeof(info->uid), 0, 0 };
	return send_answer(sock, &ans, (char *)&(info->uid));
}

int process_getpwuid(int sock, const struct nss_command *cmd, struct config *config)
{
	char *buffer = NULL;

	if (recv_command_data(sock, cmd, &buffer) < 0)
	{
		return -1;
	}

	uint32_t uid32 = *((uint32_t *)buffer);

	if (cmd->data_len != sizeof(uid32))
	{
		return reject_request(sock, cmd, EINVAL);
	}

	struct user_info *info = find_user_uid(config->users, (uid_t)uid32);

	if (info == NULL)
	{
		return reject_request(sock, cmd, ENOENT);
	}

	struct nss_answer ans = { cmd_getpwuid, strlen(info->name) + 1, 0, 0 };
	return send_answer(sock, &ans, info->name);
}

int process_getgrnam(int sock, const struct nss_command *cmd, struct config *config)
{
	char *buffer = NULL;

	if (recv_command_data(sock, cmd, &buffer) < 0)
	{
		return -1;
	}

	struct group_info *info = find_group_name(config->groups, buffer);

	if (info == NULL)
	{
		return reject_request(sock, cmd, ENOENT);
	}

	struct nss_answer ans = { cmd_getgrnam, sizeof(info->gid), 0, 0 };
	return send_answer(sock, &ans, (char *)&(info->gid));
}

int process_getgrgid(int sock, const struct nss_command *cmd, struct config *config)
{
	char *buffer = NULL;

	if (recv_command_data(sock, cmd, &buffer) < 0)
	{
		return -1;
	}

	uint32_t gid32 = *((uint32_t *)buffer);

	if (cmd->data_len != sizeof(gid32))
	{
		return reject_request(sock, cmd, EINVAL);
	}

	struct group_info *info = find_group_gid(config->groups, (gid_t)gid32);

	if (info == NULL)
	{
		return reject_request(sock, cmd, ENOENT);
	}

	struct nss_answer ans = { cmd_getgrgid, strlen(info->name) + 1, 0, 0 };
	return send_answer(sock, &ans, info->name);
}

