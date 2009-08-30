/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>

#include "attr_cache.h"
#include "buffer.h"
#include "command.h"
#include "config.h"
#include "id_lookup_client.h"
#include "instance_client.h"
#include "names.h"
#include "operations_rfs.h"
#include "sendrecv_client.h"

int _rfs_chown(struct rfs_instance *instance, const char *path, uid_t uid, gid_t gid)
{
#ifndef WITH_UGO
	/* actually dummy to keep some software happy. 
	do not replace with -EACCES or something */
	return 0;
#else
	if ((instance->client.export_opts & OPT_UGO) == 0)
	{
		/* do nothing, since this is not UGOed export */
		return 0;
	}
	
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path) + 1;
	
	const char *user = NULL;
	if (instance->client.my_uid == uid)
	{
		user = instance->config.auth_user; /* to tell server correct owner */
	}
	else if ( uid == -1 )
	{
		user = "";
	}
	else
	{
		struct passwd *pwd = getpwuid(uid);
		if (pwd == NULL)
		{
			return -EINVAL;
		}
		
		user = pwd->pw_name;
	}
	
	const char *group = NULL;
	if (instance->client.my_gid == gid)
	{
		group = instance->config.auth_user; /* to tell server correct group */
	}
	else if ( gid == -1 )
	{
		group = "";
	}
	else
	{
		struct group *grp = getgrgid(gid);
		if (grp == NULL)
		{
			return -EINVAL;
		}

		group = grp->gr_name;
	}

	char *local_user = NULL;

	if (is_nss_name(user))
	{
		local_user = local_nss_name(user, instance);
		if (local_user == NULL)
		{
			return -EINVAL;
		}
	}
	
	char *local_group = NULL;

	if (is_nss_name(group) != 0)
	{
		local_group = local_nss_name(group, instance);
		if (local_group == NULL)
		{
			if (local_user != NULL)
			{
				free(local_user);
			}
			return -EINVAL;
		}
	}

	uint32_t user_len = strlen(local_user != NULL ? local_user : user) + 1;
	uint32_t group_len = strlen(local_group != NULL ? local_group : group) + 1;

	unsigned overall_size = sizeof(user_len) 
	+ sizeof(group_len) 
	+ path_len 
	+ user_len 
	+ group_len;

	struct command cmd = { cmd_chown, overall_size };

	char *buffer = get_buffer(overall_size);
	pack(local_group != NULL ? local_group : group, group_len, buffer, 
	pack(local_user != NULL ? local_user : user, user_len, buffer, 
	pack(path, path_len, buffer, 
	pack_32(&group_len, buffer, 
	pack_32(&user_len, buffer, 0
	)))));

	if (commit_send(&instance->sendrecv, 
		queue_data(buffer, overall_size, 
		queue_cmd(&cmd, send_token(2)))) < 0)
	{
		if (local_user != NULL)
		{
			free(local_user);
		}
		
		if (local_group != NULL)
		{
			free(local_group);
		}

		free_buffer(buffer);
		return -ECONNABORTED;
	}

	if (local_user != NULL)
	{
		free(local_user);
	}
	
	if (local_group != NULL)
	{
		free(local_group);
	}

	free_buffer(buffer);

	struct answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}

	if (ans.command != cmd_chown)
	{
		return cleanup_badmsg(instance, &ans);
	}

	if ( ans.ret_errno == 0 )
	{
		delete_from_cache(instance, path);
	}

	return ans.ret == 0 ? 0 : -ans.ret_errno;
#endif
}

int _rfs_chmod(struct rfs_instance *instance, const char *path, mode_t mode)
{
#ifndef WITH_UGO
	/* actually dummy to keep some software happy. 
	do not replace with -EACCES or something */
	return 0; 
#else	
	if ((instance->client.export_opts & OPT_UGO) == 0)
	{
		/* do nothing, since this is not UGOed export */
		return 0; 
	}

	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path) + 1;
	uint32_t fmode = mode;

	unsigned overall_size = sizeof(fmode) + path_len;

	struct command cmd = { cmd_chmod, overall_size };

	char *buffer = get_buffer(overall_size);
	pack(path, path_len, buffer, 
	pack_32(&fmode, buffer, 0
	));

	if (commit_send(&instance->sendrecv, 
		queue_data(buffer, overall_size, 
		queue_cmd(&cmd, send_token(2)))) < 0)
	{
		free_buffer(buffer);
		return -ECONNABORTED;
	}

	free_buffer(buffer);

	struct answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}

	if (ans.command != cmd_chmod)
	{
		return cleanup_badmsg(instance, &ans);
	}

	if ( ans.ret_errno == 0 )
	{
		delete_from_cache(instance, path);
	}

	return ans.ret == 0 ? 0 : -ans.ret_errno;
#endif
}
