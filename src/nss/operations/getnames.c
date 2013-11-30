/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include "../../options.h"

#ifdef RFSNSS_AVAILABLE

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "../../buffer.h"
#include "../../command.h"
#include "../../config.h"
#include "../../exports.h"
#include "../../instance_client.h"
#include "../../list.h"
#include "../../operations/utils.h"
#include "../../sendrecv_client.h"
#include "../server.h"

int rfs_getnames(struct rfs_instance *instance)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}
	
	struct rfs_command cmd = { cmd_getnames, 0 };
	
	if (rfs_send_cmd(&instance->sendrecv, &cmd) == -1)
	{
		return -ECONNABORTED;
	}
	
	struct rfs_answer ans = { 0 };
	
	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}
		
	if (ans.command != cmd_getnames)
	{
		return cleanup_badmsg(instance, &ans);
	}

	if (ans.ret != 0)
	{
		return -ans.ret_errno;
	}
	
	destroy_list(&instance->nss.users_storage);
	
	if (ans.data_len > 0)
	{
		char *users = malloc(ans.data_len);
		
		if (rfs_receive_data(&instance->sendrecv, users, ans.data_len) == -1)
		{
			free(users);
			return -ECONNABORTED;
		}

		const char *user = users;
		while (user < users + ans.data_len)
		{
			size_t user_len = strlen(user) + 1;

			char *nss_user = malloc(user_len);
			memcpy(nss_user, user, user_len);

			add_to_list(&instance->nss.users_storage, nss_user);

			user += user_len;
		}

		free(users);
	}
	
	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}
		
	if (ans.command != cmd_getnames)
	{
		return cleanup_badmsg(instance, &ans);
	}

	if (ans.ret != 0)
	{
		return -ans.ret_errno;
	}

	destroy_list(&instance->nss.groups_storage);

	if (ans.data_len > 0)
	{
		char *groups = malloc(ans.data_len);
		
		if (rfs_receive_data(&instance->sendrecv, groups, ans.data_len) == -1)
		{
			free(groups);
			return -ECONNABORTED;
		}

		const char *group = groups;
		while (group < groups + ans.data_len)
		{
			size_t group_len = strlen(group) + 1;

			char *nss_group = malloc(group_len);
			memcpy(nss_group, group, group_len);

			add_to_list(&instance->nss.groups_storage, nss_group);

			group += group_len;
		}

		free(groups);
	}

	return 0;
}

#endif /* RFSNSS_AVAILABLE */
