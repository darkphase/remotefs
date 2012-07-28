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

static inline const void* compare_id_name(const void *entry, const void *name)
{
	return (strcmp(((struct id_look_ent *)entry)->name, (const char *)name) == 0 ? &((struct id_look_ent *)entry)->id : NULL) ;
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
	const void *uid = get_id(uids, name, compare_id_name);
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
	const void *gid = get_id(gids, (void *)name, compare_id_name);
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

static inline const void* compare_ids(const void *entry, const void *id)
{
	return (((struct id_look_ent *)entry)->id == *(gid_t *)id ? ((struct id_look_ent *)entry)->name : NULL) ;
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
	const void *name = get_name(uids, &uid, compare_ids);
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
	const void *name = get_name(gids, &gid, compare_ids);

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

static int put_to_ids(struct list **ids, const char *name, uint64_t id)
{
	char *dup_name = strdup(name);
	if (dup_name == NULL)
	{
		return -1;
	}

	struct id_look_ent *entry = malloc(sizeof(*entry));
	if (entry == NULL)
	{
		free(dup_name);
		return -1;
	}

	entry->name = dup_name;
	entry->id = id;

	if (add_to_list(ids, entry) == NULL)
	{
		free(dup_name);
		free(entry);

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

		if (put_to_ids(uids, pwd->pw_name, pwd->pw_uid) != 0)
		{
			endpwent();
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

		if (put_to_ids(gids, grp->gr_name, grp->gr_gid) != 0)
		{
			endgrent();
			return -1;
		}
	}
	while (grp != NULL);

	endgrent();

	return 0;
}

static inline void destroy_ids(struct list **ids)
{
	struct list *entry = *ids;

	while (entry != NULL)
	{
		free(((struct id_look_ent *)entry->data)->name);
		entry = entry->next;
	}

	destroy_list(ids);
	*ids = NULL;
}

void destroy_uids_lookup(struct list **uids)
{
	DEBUG("%s\n", "destroying uid lookup table");

	destroy_ids(uids);
}

void destroy_gids_lookup(struct list **gids)
{
	DEBUG("%s\n", "destroying gid lookup table");

	destroy_ids(gids);
}
