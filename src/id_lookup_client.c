/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <grp.h>
#include <pwd.h>

#include "config.h"
#include "id_lookup_client.h"

uid_t lookup_user(const char *name)
{
	struct passwd *pwd = getpwnam(name);

	if (pwd != NULL)
	{
		return pwd->pw_uid;
	}

	pwd = getpwnam("nobody");
	if (pwd != NULL)
	{
		return pwd->pw_uid;
	}
	
	return 0; /* default to root */
}

gid_t lookup_group(const char *name, const char *user_name)
{
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

	/* try to default to standard groups defining noone */
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

const char* lookup_uid(uid_t uid)
{
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

const char* lookup_gid(gid_t gid, uid_t uid)
{
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

