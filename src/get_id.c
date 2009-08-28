/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <pwd.h>
#include <grp.h>

#include "config.h"
#include "get_id.h"

uid_t get_free_uid(struct config *config, const char *user)
{
	uid_t uid = config->last_uid + 1;

	if (uid < START_UID)
	{
		uid = (uid_t)(START_UID + 1);
	}

	while (getpwuid(uid) != NULL
	&& uid >= START_UID)
	{
		++uid;
	}

	DEBUG("uid %d is free\n", uid);

	if (uid < START_UID)
	{
		return (uid_t)-1;
	}

	config->last_uid = uid;

	return uid;
}

gid_t get_free_gid(struct config *config, const char *group)
{
	gid_t gid = config->last_gid + 1;

	if (gid < START_GID)
	{
		gid = (gid_t)(START_GID + 1);
	}

	while (getgrgid(gid) != NULL
	&& gid >= START_GID)
	{
		DEBUG("cheked gid: %d\n", gid);
		++gid;
	}

	if (gid < START_GID)
	{
		return (gid_t)-1;
	}

	DEBUG("ok, gid is free: %d\n", gid);

	config->last_gid = gid;

	return gid;
}

