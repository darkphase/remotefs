/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <list.h>
#include <nss/client.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "client_for_server.h"
#include "get_id.h"
#include "manage_server.h"

struct user_info* find_user_name(const struct list *root, const char *name)
{
	const struct list *item = root;

	while (item != NULL)
	{
		struct user_info *info = (struct user_info *)item->data;

		if (strcmp(name, info->name) == 0)
		{
			return info;
		}

		item = item->next;
	}

	return NULL;
}

struct user_info* find_user_uid(const struct list *root, const uid_t uid)
{
	const struct list *item = root;

	while (item != NULL)
	{
		struct user_info *info = (struct user_info *)item->data;

		if (uid == info->uid)
		{
			return info;
		}

		item = item->next;
	}

	return NULL;
}

struct group_info* find_group_name(const struct list *root, const char *name)
{
	const struct list *item = root;

	while (item != NULL)
	{
		struct group_info *info = (struct group_info *)item->data;

		if (strcmp(name, info->name) == 0)
		{
			return info;
		}

		item = item->next;
	}

	return NULL;
}

struct group_info* find_group_gid(const struct list *root, const gid_t gid)
{
	const struct list *item = root;

	while (item != NULL)
	{
		struct group_info *info = (struct group_info *)item->data;

		if (gid == info->gid)
		{
			return info;
		}

		item = item->next;
	}

	return NULL;
}

int add_user(struct list **root, const char *user, uid_t uid)
{
	struct user_info *name_info = find_user_name(*root, user);

	if (name_info != NULL)
	{
		name_info->uid = uid;
		return 0;
	}

	struct user_info *user_rec = malloc(sizeof(*user_rec));

	user_rec->name = strdup(user);
	user_rec->uid = uid;

	if (add_to_list(root, user_rec) == NULL)
	{
		free(user_rec->name);
		free(user_rec);
		
		return -1;
	}

	DEBUG("added user %s with uid %d (%p)\n", user, uid, (void *)user_rec);

	return 0;
}

int add_group(struct list **root, const char *group, gid_t gid)
{
	struct group_info *name_info = find_group_name(*root, group);
	if (name_info != NULL)
	{
		name_info->gid = gid;
		return 0;
	}

	struct group_info *group_rec = malloc(sizeof(*group_rec));

	group_rec->name = strdup(group);
	group_rec->gid = gid;

	if (add_to_list(root, group_rec) == NULL)
	{
		free(group_rec->name);
		free(group_rec);
		
		return -1;
	}
	
	DEBUG("added group %s with gid %d\n", group, gid);

	return 0;
}

int add_rfs_server(struct config *config, const char *server_name)
{
	DEBUG("adding server %s\n", server_name);

	struct list *users = NULL;
	struct list *groups = NULL;

	if (nss_get_users(server_name, &users) != 0)
	{
		return -EIO;
	}

	if (nss_get_groups(server_name, &groups) != 0)
	{
		destroy_list(&users);
		return -EIO;
	}

	uid_t myuid = (config->allow_other ? -1 : getuid());

	struct list *user = users;
	struct list *group = groups;
	while (user != NULL)
	{
		uid_t uid = get_free_uid(config, user->data);
		if (uid == (uid_t)-1)
		{
			goto error;
		}

		char full_name[RFS_NSS_MAX_NAME + 1] = { 0 };
		if (snprintf(full_name, sizeof(full_name), "%s@%s", (const char *)user->data, server_name) >= sizeof(full_name))
		{
			goto error;
		}

		int adduser_ret = rfsnss_adduser(full_name, uid, myuid);
		if (adduser_ret < 0)
		{
			destroy_list(&users);
			destroy_list(&groups);
			return adduser_ret;
		}

		user = user->next;
	}

	while (group != NULL)
	{
		gid_t gid = get_free_gid(config, group->data);
		if (gid == (gid_t)-1)
		{
			goto error;
		}
		
		char full_name[RFS_NSS_MAX_NAME + 1] = { 0 };
		if (snprintf(full_name, sizeof(full_name), "%s@%s", (const char *)group->data, server_name) >= sizeof(full_name))
		{
			goto error;
		}

		int addgroup_ret = rfsnss_addgroup(full_name, gid, myuid);

		if (addgroup_ret < 0)
		{
			destroy_list(&users);
			destroy_list(&groups);
			return addgroup_ret;
		}

		group = group->next;
	}

	destroy_list(&users);
	destroy_list(&groups);
	
	DEBUG("%s\n", "ok");

	return 0;

error:
	destroy_list(&users);
	destroy_list(&groups);

	return -EIO;
}

