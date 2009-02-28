/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <grp.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "id_lookup.h"
#include "buffer.h"
#include "list.h"

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

