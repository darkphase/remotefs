/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <arpa/inet.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../../auth.h"
#include "../../buffer.h"
#include "../../command.h"
#include "../../config.h"
#include "../../exports.h"
#include "../../handling.h"
#include "../../id_lookup.h"
#include "../../instance_server.h"
#include "../../passwd.h"
#include "../../sendrecv_server.h"

static int setup_groups_by_uid(uid_t uid)
{
	struct passwd *pwd = getpwuid(uid);

	if (pwd == NULL)
	{
		return -ENOENT;
	}

	DEBUG("initing groups for %s\n", pwd->pw_name);

	errno = 0;
	if (initgroups(pwd->pw_name, pwd->pw_gid) != 0)
	{
		return -errno;
	}

#ifdef RFS_DEBUG
	gid_t user_groups[255] = { 0 };
	int user_groups_count = getgroups(sizeof(user_groups) / sizeof(*user_groups), user_groups);
	if (user_groups_count > 0)
	{
		int i = 0; for (i = 0; i < user_groups_count; ++i)
		{
			struct group *grp = getgrgid(user_groups[i]);
			if (grp != NULL)
			{
				DEBUG("member of %s\n", grp->gr_name);
			}
		}
	}
#endif

	return 0;
}

static int setup_export_user(uid_t user_uid, gid_t user_gid)
{
	/* always setup groups before setting uid :) */
	
	DEBUG("setting process uid and gid according to uid %d, gid: %d\n", user_uid, user_gid);

	if (setgid(user_gid) != 0
	|| setuid(user_uid) != 0)
	{
		return -EACCES;
	}
	
	return 0;
}

static int setup_export_groups(uid_t user_uid)
{
	DEBUG("setting process groups according to uid %d\n", user_uid);

	if (setup_groups_by_uid(user_uid) != 0)
	{
		return -EACCES;
	}
	
	return 0;
}

int _handle_changepath(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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
	
	if (strlen(buffer) + 1 != cmd->data_len)
	{
		free(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	while (strlen(buffer) > 1 /* do not remove first '/' */
	&& buffer[strlen(buffer) - 1] == '/')
	{
		buffer[strlen(buffer) - 1] = 0;
	}
	
	const char *path = buffer;
	
	DEBUG("client want to change path to %s\n", path);

	const struct rfs_export *export_info = strlen(path) > 0 ? get_export(instance->exports.list, path) : NULL;
#ifndef WITH_IPV6
	if (export_info == NULL 
	|| check_permissions(instance, export_info, inet_ntoa(client_addr->sin_addr)) != 0)
	{
		free(buffer);
		return reject_request(instance, cmd, EACCES) == 0 ? 1 : -1;
	}
#else
	const struct sockaddr_in6 *sa6 = (struct sockaddr_in6*)client_addr;
	char straddr[INET6_ADDRSTRLEN];
	if (client_addr->sin_family == AF_INET)
	{
		inet_ntop(AF_INET, &client_addr->sin_addr, straddr,sizeof(straddr));
	}
	else
	{
		inet_ntop(AF_INET6, &sa6->sin6_addr, straddr,sizeof(straddr));
	}
	
	if (export_info == NULL 
	|| check_permissions(instance, export_info, straddr) != 0) 
	{
		free(buffer);
		return reject_request(instance, cmd, EACCES) == 0 ? 1 : -1;
	}
#endif

	/* by default use running process uid and gid */
	uid_t user_uid = geteuid();
	gid_t user_gid = getegid();

	/* if user= is specified, use that uid and primary gid of that user */
	if (export_info->export_uid != -1)
	{
		struct passwd *pwd = getpwuid(export_info->export_uid);
		if (pwd == NULL)
		{
			free(buffer);
			return reject_request(instance, cmd, EACCES) == 0 ? 1 : -1;
		}

		user_uid = pwd->pw_uid;
		user_gid = pwd->pw_gid;
	}

#ifdef WITH_UGO
	/* if we're going into UGO, then user_uid and user_gid should 
	point to logged user */
	if (instance->server.auth_user != NULL 
	&& (export_info->options & OPT_UGO) != 0)
	{
		struct passwd *pwd = getpwnam(instance->server.auth_user);
		if (pwd != NULL)
		{
			user_uid = pwd->pw_uid;
			user_gid = pwd->pw_gid;
		}
		else
		{
			free(buffer);
			return reject_request(instance, cmd, EACCES) == 0 ? 1 : -1;
		}

		create_uids_lookup(&instance->id_lookup.uids);
		create_gids_lookup(&instance->id_lookup.gids);
	}
#endif

	/* we need to setup groups before chroot() */
	if (setup_export_groups(user_uid) != 0)
	{
		free(buffer);

		destroy_uids_lookup(&instance->id_lookup.uids);
		destroy_gids_lookup(&instance->id_lookup.gids);
			
		return reject_request(instance, cmd, EACCES) == 0 ? 1 : -1;
	}

	errno = 0;
	int result = chroot(path);

	struct answer ans = { cmd_changepath, 0, result, errno };
	
	free(buffer);
	
	if (result == 0)
	{
		int setup_errno = setup_export_user(user_uid, user_gid);
		if (setup_errno != 0)
		{
			destroy_uids_lookup(&instance->id_lookup.uids);
			destroy_gids_lookup(&instance->id_lookup.gids);
			
			return reject_request(instance, cmd, -setup_errno) == 0 ? 1 : -1;
		}
	}

	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	if (result == 0)
	{	
		instance->server.directory_mounted = 1;
		instance->server.mounted_export = malloc(sizeof(*instance->server.mounted_export));
		memcpy(instance->server.mounted_export, export_info, sizeof(*instance->server.mounted_export));
		
		release_passwords(&instance->passwd.auths);
		release_exports(&instance->exports.list);
	}
	else
	{
		destroy_uids_lookup(&instance->id_lookup.uids);
		destroy_gids_lookup(&instance->id_lookup.gids);
	}

	return 0;
}
