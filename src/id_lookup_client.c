#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>

#include "rfs_nsslib.h"
#include "config.h"
#include "id_lookup.h"

static char *host_name = NULL;
void init_nss_host_name(char *name)
{
	host_name = name;
}

static inline char *make_name(const char *name)
{
	char *qualified_name = NULL;
	int len;
	if ( host_name == NULL )
	{
		return NULL;
	}
	len = strlen(name)+strlen(host_name)+3;
	qualified_name = calloc(len,1);
	if ( qualified_name )
	{
		snprintf(qualified_name,len, "%s@%s",name,host_name);
	}
	return qualified_name;
}

uid_t get_uid(const struct list *uids, const char *name)
{
	struct passwd *pwd = NULL;
	char *qualified_name = make_name(name);
	if ( qualified_name )
	{
		pwd = getpwnam(qualified_name);
		free(qualified_name);
	}

	if ( pwd == NULL )
	{
		pwd = getpwnam(name);
		if ( pwd == NULL )
		{
			pwd = getpwnam("nobody");
		}
	}
	
	return pwd != NULL ? pwd->pw_uid : 0;
}

gid_t get_gid(const struct list *gids, const char *name)
{
	struct group *grp = NULL;
	char *qualified_name = make_name(name);
	if ( qualified_name )
	{
		grp = getgrnam(qualified_name);
		free(qualified_name);
	}
	
	if (grp == NULL )
	{
		grp = getgrnam(name);
	}
	
	if (grp == NULL)
	{
		grp = getgrnam("nogroup");
	}
	
	if (grp == NULL)
	{
		grp = getgrnam("nobody");
	}
	
	return grp != NULL ? grp->gr_gid : 0;
}

const char* get_uid_name(const struct list *uids, uid_t uid)
{
	struct passwd *pwd = getpwuid(uid);
	const char *user = pwd ? pwd->pw_name : NULL;
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

const char* get_gid_name(const struct list *gids, gid_t gid)
{
	struct group *grp = getgrgid(gid);
	const char *group = grp ? grp->gr_name : NULL;
	if (group == NULL
	&& getgrnam("nogroup") != NULL)
	{
		group = "nogroup";
	}
	
	if (group == NULL
	&& getgrnam("nobody"))
	{
		group = "nobody";
	}
	
	if (group == NULL)
	{
		struct passwd *pwd = getpwuid(gid);
		group = pwd ? pwd->pw_name : NULL;
	}
	
	if (group == NULL)
	{
		group = "root"; 
	}
	
	return group;
}

void destroy_uids_lookup(struct list **uids)
{
	return;
}

void destroy_gids_lookup(struct list **gids)
{
	return;
}

int create_gids_lookup(struct list **gids)
{
	return 0;
}

int create_uids_lookup(struct list **uids)
{
	return 0;
}

uid_t lookup_user(const struct list *uids, const char *name)
{
	return get_uid(uids, name);
}

gid_t lookup_group(const struct list *gids, const char *name, const char *user_name)
{
	return get_gid(gids, name);
}
