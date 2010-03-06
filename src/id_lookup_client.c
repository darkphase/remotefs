/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <grp.h>
#include <pwd.h>

#include "config.h"
#include "id_lookup.h"
#include "id_lookup_client.h"
#include "instance_client.h"

uid_t lookup_user(const struct rfs_instance *instance, const char *name)
{
	DEBUG("looking up user %s\n", name);

	/* first check id lookup list */
	uid_t uid = get_uid(instance->id_lookup.uids, name);
	if (uid != (uid_t)(-1))
	{
		return uid;
	}

	/* fallback to system user lookup */
	struct passwd *pwd = getpwnam(name);

	if (pwd != NULL)
	{
		return pwd->pw_uid;
	}

	/* try to fallback to nobody */
	pwd = getpwnam("nobody");
	if (pwd != NULL)
	{
		return pwd->pw_uid;
	}
	
	return 0; /* default to root */
}

gid_t lookup_group(const struct rfs_instance *instance, const char *name, const char *user_name)
{
	DEBUG("looking up group %s\n", name);

	/* first check id lookup list */
	gid_t gid = get_gid(instance->id_lookup.gids, name);
	if (gid != (gid_t)(-1))
	{
		return gid;
	}

	/* fallback to system group lookup */
	struct group *grp = getgrnam(name);

	if (grp != NULL)
	{
		return grp->gr_gid; 
	}

	/* next, try to find group with the same name as user 
	(and default group to that) */
	if (user_name != NULL)
	{
		grp = getgrnam(user_name);
		if (grp != NULL)
		{
			return grp->gr_gid;
		}
	}

	/* try to default to standard groups defining none */
	grp = getgrnam("nogroup");
	if (grp != NULL)
	{
		return grp->gr_gid; 
	}

	grp = getgrnam("nobody");
	if (grp != NULL)
	{
		return grp->gr_gid; 
	}

	return 0; /* default to root */
}

const char* lookup_uid(const struct rfs_instance *instance, uid_t uid)
{
	/* first check lookup list */
	const char *name = get_uid_name(instance->id_lookup.uids, uid);
	if (name != NULL)
	{
		return name;
	}

	/* fallback to system uid lookup */
	struct passwd *pwd = getpwuid(uid);
	if (pwd != NULL)
	{
		return pwd->pw_name;
	}

	/* default to nobody if possible */
	if (getpwnam("nobody") != NULL)
	{
		return "nobody";
	}
	
	return "root"; /* default to root */
}

const char* lookup_gid(const struct rfs_instance *instance, gid_t gid, uid_t uid)
{
	/* first chekc lookup list */
	const char *name = get_gid_name(instance->id_lookup.gids, gid);
	if (name != NULL)
	{
		return name;
	}

	/* fallback to system gid lookup */
	struct group *grp = getgrgid(gid);
	if (grp != NULL)
	{
		return grp->gr_name;
	}

	/* try to find group with the same name as user */
	if (uid != (uid_t)(-1))
	{
		struct passwd *pwd = getpwuid(uid);
		if (pwd != NULL)
		{
			grp = getgrnam(pwd->pw_name);
			if (grp != NULL)
			{
				return grp->gr_name;
			}
		}
	}

	/* try to default to standard groups defining noone */
	if (getgrnam("nogroup") != NULL)
	{
		return "nogroup";
	}

	if (getgrnam("nobody") != NULL)
	{
		return "nobody";
	}
	
	return "root"; /* default to root */
}
