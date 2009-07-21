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

typedef const void* (*compare_func)(const void *entry, const void *value);
typedef void (*destroy_func)(void *data);
typedef void* (*alloc_func)(char *name, const void *id);

static inline const void* compare_uid_name(const void *entry, const void *name)
{
	return (strcmp(((struct uid_look_ent *)entry)->name, (const char *)name) == 0 ? &((struct uid_look_ent *)entry)->uid : NULL) ;
}

static inline const void* compare_gid_name(const void *entry, const void *name)
{
	return (strcmp(((struct gid_look_ent *)entry)->name, (const char *)name) == 0 ? &((struct gid_look_ent *)entry)->gid : NULL) ;
}

static const void* get_id(const struct list *ids, const char *name, compare_func comparator)
{
	const struct list *entry = ids;
	
	while (entry != NULL)
	{
		const void *id = comparator(entry->data, name);
		if (id != NULL)
		{
			return id;
		}
		
		entry = entry->next;
	}

	return NULL;
}

uid_t get_uid(const struct list *uids, const char *name)
{
	const void *uid = get_id(uids, name, compare_uid_name);
	if (uid != NULL)
	{
		return *(uid_t *)uid;
	}
	
	struct passwd *pwd = getpwnam(name);
	if (pwd != NULL)
	{
		return pwd->pw_uid;
	}

	return (uid_t)-1;
}

gid_t get_gid(const struct list *gids, const char *name)
{
	const void *gid = get_id(gids, (void *)name, compare_gid_name);
	if (gid != NULL)
	{
		return *(gid_t *)gid;
	}
	
	struct group *grp = getgrnam(name);
	if (grp != NULL)
	{
		return grp->gr_gid;
	}
	
	return (gid_t)-1;
}

static inline const void* compare_gids(const void *entry, const void *gid)
{
	return (((struct gid_look_ent *)entry)->gid == *(gid_t *)gid ? ((struct gid_look_ent *)entry)->name : NULL) ;
}

static inline const void* compare_uids(const void *entry, const void *uid)
{
	return (((struct uid_look_ent *)entry)->uid == *(uid_t *)uid ? ((struct uid_look_ent *)entry)->name : NULL) ;
}

static const void* get_name(const struct list *uids, void *id, compare_func comparator)
{
	const struct list *entry = uids;
	
	while (entry != NULL)
	{
		const void *name = comparator(entry->data, id);
		if (name != NULL)
		{
			return name;
		}
		
		entry = entry->next;
	}

	return NULL;
}

const char* get_uid_name(const struct list *uids, uid_t uid)
{
	const void *name = get_name(uids, &uid, compare_uids);
	if (name != NULL)
	{
		return (const char *)name;
	}
	
	struct passwd *pwd = getpwuid(uid);
	if (pwd != NULL)
	{
		return pwd->pw_name;
	}
	
	return NULL;
}

const char* get_gid_name(const struct list *gids, gid_t gid)
{
	const void *name = get_name(gids, &gid, compare_gids);

	if (name != NULL)
	{
		return (const char *)name;
	}
	
	struct group *grp = getgrgid(gid);
	if (grp != NULL)
	{
		return grp->gr_name;
	}

	return NULL;
}

static inline void* alloc_uid_ent(char *name, const void *id)
{
	struct uid_look_ent *entry = get_buffer(sizeof(*entry));
	if (entry == NULL)
	{
		return NULL;
	}

	entry->name = name;
	entry->uid = *(uid_t *)id;

	return entry;
}

static inline void* alloc_gid_ent(char *name, const void *id)
{
	struct gid_look_ent *entry = get_buffer(sizeof(*entry));
	if (entry == NULL)
	{
		return NULL;
	}

	entry->name = name;
	entry->gid = *(gid_t *)id;

	return entry;
}

static int put_to_ids(struct list **ids, const char *name, const void *id, alloc_func allocator)
{
	char *dup_name = strdup(name);
	if (dup_name == NULL)
	{
		return -1;
	}

	void *entry = allocator(dup_name, id);
	if (entry == NULL)
	{
		free(dup_name);
		return -1;
	}
	
	if (add_to_list(ids, entry) == NULL)
	{
		free(dup_name);
		free_buffer(entry);
		
		return -1;
	}

	return 0;
}

int create_uids_lookup(struct list **uids)
{
	DEBUG("%s\n", "creating uid lookup table");

	struct passwd *pwd = NULL;
	
	setpwent();

	do
	{
		pwd = getpwent();
		if (pwd == NULL)
		{
			break;
		}
		
		if (put_to_ids(uids, pwd->pw_name, (const void *)&(pwd->pw_uid), alloc_uid_ent) != 0)
		{
			return -1;
		}
	}
	while (pwd != NULL);
	
	endpwent();

	return 0;
}

int create_gids_lookup(struct list **gids)
{
	DEBUG("%s\n", "creating gid lookup table");
	
	struct group *grp = NULL;

	setgrent();

	do
	{
		grp = getgrent();
		if (grp == NULL)
		{
			break;
		}
		
		if (put_to_ids(gids, grp->gr_name, (const void *)&(grp->gr_gid), alloc_gid_ent) != 0)
		{
			return -1;
		}
	}
	while (grp != NULL);
	
	endgrent();

	return 0;
}

static inline void destroy_uid(void *entry)
{
	free(((struct uid_look_ent *)entry)->name);
}

static inline void destroy_gid(void *entry)
{
	free(((struct gid_look_ent *)entry)->name);
}

static inline void destroy_ids(struct list **ids, destroy_func destroyer)
{
	struct list *entry = *ids;

	while (entry != NULL)
	{
		destroyer(entry->data);
		
		entry = entry->next;
	}
	
	destroy_list(ids);
	*ids = NULL;
}

void destroy_uids_lookup(struct list **uids)
{
	DEBUG("%s\n", "destroying uid lookup table");

	destroy_ids(uids, destroy_uid);
}

void destroy_gids_lookup(struct list **gids)
{
	DEBUG("%s\n", "destroying gid lookup table");
	
	destroy_ids(gids, destroy_gid);
}

