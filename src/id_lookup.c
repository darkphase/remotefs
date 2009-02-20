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

static int put_to_uids(struct list **uids, const char *name, const uid_t uid)
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
	
	if (add_to_list(uids, entry) == NULL)
	{
		free((void*)entry->name);
		free_buffer(entry);
		
		return -1;
	}
	
	return 0;
}

uid_t get_uid(const struct list *uids, const char *name)
{
	const struct list *entry = uids;
	
	while (entry != NULL)
	{
		const struct uid_look_ent *item = (const struct uid_look_ent *)entry->data;
		if (strcmp(item->name, name) == 0)
		{
			return item->uid;
		}
		
		entry = entry->next;
	}
	
	return (uid_t)-1;
}

const char* get_uid_name(const struct list *uids, uid_t uid)
{
	const struct list *entry = uids;
	
	while (entry != NULL)
	{
		const struct uid_look_ent *item = (const struct uid_look_ent *)entry->data;
		if (item->uid == uid)
		{
			return item->name;
		}
		
		entry = entry->next;
	}
	
	return NULL;
}

static int put_to_gids(struct list **gids, const char *name, const gid_t gid)
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
	
	if (add_to_list(gids, entry) == NULL)
	{
		free((void*)entry->name);
		free_buffer(entry);
		
		return -1;
	}
	
	return 0;
}

gid_t get_gid(const struct list *gids, const char *name)
{
	const struct list *entry = gids;
	
	while (entry != NULL)
	{
		const struct gid_look_ent *item = (const struct gid_look_ent *)entry->data;
		if (strcmp(item->name, name) == 0)
		{
			return item->gid;
		}
		
		entry = entry->next;
	}
	
	return (gid_t)-1;
}

const char* get_gid_name(const struct list *gids, gid_t gid)
{
	const struct list *entry = gids;
	
	while (entry != NULL)
	{
		const struct gid_look_ent *item = (const struct gid_look_ent *)entry->data;
		if (item->gid == gid)
		{
			return item->name;
		}
		
		entry = entry->next;
	}
	
	return NULL;
}

int create_uids_lookup(struct list **uids)
{
	DEBUG("%s\n", "creating uid lookup table");

	struct passwd *pwd = NULL;
#if defined FREEBSD
	setpwent();
#endif
	do
	{
		pwd = getpwent();
		if (pwd == NULL)
		{
			break;
		}
		
		if (put_to_uids(uids, pwd->pw_name, pwd->pw_uid) != 0)
		{
			return -1;
		}
	}
	while (pwd != NULL);
	
#if defined FREEBSD
	endpwent();
#endif
	return 0;
}

int create_gids_lookup(struct list **gids)
{
	DEBUG("%s\n", "creating gid lookup table");
	
	struct group *grp = NULL;
#if defined FREEBSD
	setgrent();
#endif
	do
	{
		grp = getgrent();
		if (grp == NULL)
		{
			break;
		}
		
		if (put_to_gids(gids, grp->gr_name, grp->gr_gid) != 0)
		{
			return -1;
		}
	}
	while (grp != NULL);
	
#if defined FREEBSD
	endgrent();
#endif
	return 0;
}

void destroy_uids_lookup(struct list **uids)
{
	DEBUG("%s\n", "destroying uid lookup table");
	
	struct list *entry = *uids;
	
	while (entry != NULL)
	{
		struct uid_look_ent *item = (struct uid_look_ent *)entry->data;
		free((void*)item->name);
		item->name = NULL;
		
		entry = entry->next;
	}
	
	destroy_list(uids);
	*uids = NULL;
}

void destroy_gids_lookup(struct list **gids)
{
	DEBUG("%s\n", "destroying gid lookup table");
	
	struct list *entry = *gids;
	
	while (entry != NULL)
	{
		struct gid_look_ent *item = (struct gid_look_ent *)(entry->data);
		free((void*)item->name);
		
		entry = entry->next;
	}
	
	destroy_list(gids);
	*gids = NULL;
}

uid_t lookup_user(const struct list *uids, const char *name)
{
	uid_t uid = get_uid(uids, name);
	if (uid == (uid_t)-1)
	{
		uid = get_uid(uids, "nobody");
	}
	
	return uid != (uid_t)-1 ? uid : 0;
}

gid_t lookup_group(const struct list *gids, const char *name, const char *user_name)
{
	gid_t gid = get_gid(gids, name);
	
	if (gid == (gid_t)-1
	&& user_name != NULL)
	{
		gid = get_gid(gids, user_name);
	}
	
	if (gid == (gid_t)-1)
	{
		gid = get_gid(gids, "nogroup");
	}
	
	if (gid == (gid_t)-1)
	{
		gid = get_gid(gids, "nobody");
	}
	
	return gid != (gid_t)-1 ? gid : 0;
}

const char* lookup_uid(const struct list *uids, uid_t uid)
{
	const char *user = get_uid_name(uids, uid);
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

const char* lookup_gid(const struct list *gids, gid_t gid, const struct list *uids, uid_t uid)
{
	const char *group = get_gid_name(gids, gid);
	if (group == NULL
	&& get_gid(gids, "nogroup"))
	{
		group = "nogroup";
	}
	
	if (group == NULL
	&& get_gid(gids, "nobody"))
	{
		group = "nobody";
	}
	
	if (group == NULL)
	{
		group = get_uid_name(uids, uid);
	}
	
	if (group == NULL)
	{
		group = "root"; 
	}
	
	return group;
}

#if defined WITH_NSS
const char *get_user_name(struct list **uid_list, int min)
{
	while ( uid_list && *uid_list )
	{
		struct list *entry = *uid_list;
		const struct uid_look_ent *item = (const struct uid_look_ent *)entry->data;
		if ( item->uid >= (uid_t)min )
		{
			*uid_list = entry->next;
			return item->name;
		}
		else
		{
			*uid_list = entry->next;
		}
	}
	return NULL;
}

const char *get_group_name(struct list **gid_list, int min)
{
	
	while ( gid_list && *gid_list )
	{
		struct list *entry = *gid_list;
		const struct gid_look_ent *item = (const struct gid_look_ent *)entry->data;
		if ( item->gid >= (gid_t)min )
		{
			*gid_list = entry->next;
			return item->name;
		}
		else
		{
			*gid_list = entry->next;
		}
	}
	return NULL;
}
#endif
