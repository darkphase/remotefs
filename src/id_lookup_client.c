/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include "config.h"
#include "id_lookup_client.h"

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
