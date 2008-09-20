/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <stdlib.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>

#include "config.h"
#include "id_lookup.h"
#include "buffer.h"
#include "list.h"

struct uid_look_ent
{
	const char *name;
	uid_t uid;
};

struct gid_look_ent
{
	const char *name;
	gid_t gid;
};

static struct list *uids_root = NULL;
static struct list *gids_root = NULL;

int put_to_uids(const char *name, const uid_t uid)
{
	struct uid_look_ent *entry = get_buffer(sizeof(*entry));
	if (entry == NULL)
	{
		return -1;
	}
	
	const char *dup_name = strdup(name);
	if (dup_name == NULL)
	{
		free_buffer(entry);
		return -1;
	}
	
	entry->name = dup_name;
	entry->uid = uid;
	
	struct list *added = add_to_list(uids_root, entry);
	
	if (added == NULL)
	{
		free((void*)entry->name);
		free_buffer(entry);
		
		return -1;
	}
	
	if (uids_root == NULL)
	{
		uids_root = added;
	}
	
	return 0;
}

uid_t get_uid(const char *name)
{
	struct list *entry = uids_root;
	
	while (entry != NULL)
	{
		struct uid_look_ent *item = (struct uid_look_ent *)entry->data;
		if (strcmp(item->name, name) == 0)
		{
			return item->uid;
		}
		
		entry = entry->next;
	}
	
	return (uid_t)-1;
}

const char* get_uid_name(uid_t uid)
{
	struct list *entry = uids_root;
	
	while (entry != NULL)
	{
		struct uid_look_ent *item = (struct uid_look_ent *)entry->data;
		if (item->uid == uid)
		{
			return item->name;
		}
		
		entry = entry->next;
	}
	
	return NULL;
}

int put_to_gids(const char *name, const gid_t gid)
{
	struct gid_look_ent *entry = get_buffer(sizeof(*entry));
	if (entry == NULL)
	{
		return -1;
	}
	
	const char *dup_name = strdup(name);
	if (dup_name == NULL)
	{
		free_buffer(entry);
		return -1;
	}
	
	entry->name = dup_name;
	entry->gid = gid;
	
	struct list *added = add_to_list(gids_root, entry);
	
	if (added == NULL)
	{
		free((void*)entry->name);
		free_buffer(entry);
		
		return -1;
	}
	
	if (gids_root == NULL)
	{
		gids_root = added;
	}
	
	return 0;
}

gid_t get_gid(const char *name)
{
	struct list *entry = gids_root;
	
	while (entry != NULL)
	{
		struct gid_look_ent *item = (struct gid_look_ent *)entry->data;
		if (strcmp(item->name, name) == 0)
		{
			return item->gid;
		}
		
		entry = entry->next;
	}
	
	return (gid_t)-1;
}

const char* get_gid_name(gid_t gid)
{
	struct list *entry = gids_root;
	
	while (entry != NULL)
	{
		struct gid_look_ent *item = (struct gid_look_ent *)entry->data;
		if (item->gid == gid)
		{
			return item->name;
		}
		
		entry = entry->next;
	}
	
	return NULL;
}

int create_uids_lookup()
{
	DEBUG("%s\n", "creating uid lookup table");

	struct passwd *pwd = NULL;
	do
	{
		pwd = getpwent();
		if (pwd == NULL)
		{
			break;
		}
		
		if (put_to_uids(pwd->pw_name, pwd->pw_uid) != 0)
		{
			return -1;
		}
	}
	while (pwd != NULL);
	
	return 0;
}

int create_gids_lookup()
{
	DEBUG("%s\n", "creating gid lookup table");
	
	struct group *grp = NULL;
	do
	{
		grp = getgrent();
		if (grp == NULL)
		{
			break;
		}
		
		if (put_to_gids(grp->gr_name, grp->gr_gid) != 0)
		{
			return -1;
		}
	}
	while (grp != NULL);
	
	return 0;
}

void destroy_uids_lookup()
{
	DEBUG("%s\n", "destroying uid lookup table");
	
	struct list *entry = uids_root;
	
	while (entry != NULL)
	{
		struct uid_look_ent *item = (struct uid_look_ent *)entry->data;
		free((void*)item->name);
		item->name = NULL;
		
		entry = entry->next;
	}
	
	destroy_list(uids_root);
	uids_root = NULL;
}

void destroy_gids_lookup()
{
	DEBUG("%s\n", "destroying gid lookup table");
	
	struct list *entry = gids_root;
	
	while (entry != NULL)
	{
		struct gid_look_ent *item = (struct gid_look_ent *)(entry->data);
		free((void*)item->name);
		
		entry = entry->next;
	}
	
	destroy_list(gids_root);
	gids_root = NULL;
}

uid_t lookup_user(const char *name)
{
	uid_t uid = get_uid(name);
	if (uid == (uid_t)-1)
	{
		uid = get_uid("nobody");
	}
	
	return uid != (uid_t)-1 ? uid : 0;
}

gid_t lookup_group(const char *name, const char *user_name)
{
	gid_t gid = get_gid(name);
	
	if (gid == (gid_t)-1
	&& user_name != NULL)
	{
		gid = get_gid(user_name);
	}
	
	if (gid == (gid_t)-1)
	{
		gid = get_gid("nogroup");
	}
	
	if (gid == (gid_t)-1)
	{
		gid = get_gid("nobody");
	}
	
	return gid != (gid_t)-1 ? gid : 0;
}

const char* lookup_uid(uid_t uid)
{
	const char *user = get_uid_name(uid);
	if (user == NULL)
	{
		user = "nobody";
	}
	
	if (user == NULL)
	{
		user = "root";
	}
	
	return user;
}

const char* lookup_gid(gid_t gid, uid_t uid)
{
	const char *group = get_gid_name(gid);
	if (group == NULL
	&& get_gid("nogroup"))
	{
		group = "nogroup";
	}
	
	if (group == NULL
	&& get_gid("nobody"))
	{
		group = "nobody";
	}
	
	if (group == NULL)
	{
		group = get_uid_name(uid);
	}
	
	if (group == NULL)
	{
		group = "root"; 
	}
	
	return group;
}
