/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include "../options.h"

#ifdef ACL_OPERATIONS_AVAILABLE

#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>

#include "nss_resolve.h"
#include "../buffer.h"
#include "../config.h"
#include "../instance_client.h"

static uint32_t get_id(acl_tag_t tag, const char *name, size_t name_len)
{
	char *dup_name = buffer_dup_str(name, name_len);
	
	DEBUG("getting name: %s (%ld)\n", dup_name, (long)strlen(dup_name));

	uint32_t id = ACL_UNDEFINED_ID;

	switch (tag)
	{
	case ACL_USER:
		{
		errno = 0;
		struct passwd *pwd = getpwnam(name);
		DEBUG("%s\n", strerror(errno));
		if (pwd != NULL)
		{
			id = (uint32_t)(pwd->pw_uid);
		}
		}
		break;
	case ACL_GROUP:
		{
		struct group *grp = getgrnam(name);
		if (grp != NULL)
		{
			id = (uint32_t)(grp->gr_gid);
		}
		}
		break;
	}

	DEBUG("got id: %lu\n", (unsigned long)(id));

	free_buffer(dup_name);
	return id;
}

uint32_t nss_resolve(acl_tag_t tag, const char *name, size_t name_len, void *instance_casted)
{
	if (name == NULL 
	|| name_len == 0)
	{
		return ACL_UNDEFINED_ID;
	}

	struct rfs_instance *instance = (struct rfs_instance *)(instance_casted);

	DEBUG("resolving ACL name: %s\n", name);

	if (tag == ACL_USER 
	&& strncmp(name, instance->config.auth_user, name_len) == 0)
	{
		return instance->client.my_uid;
	}

	const char *fixed_name = name;
	size_t fixed_name_len = 0;
	char *allocated_name = NULL;
	uint32_t id = ACL_UNDEFINED_ID;

	DEBUG("nss resolve: name: %s, name_len: %lu\n", name, (unsigned long)name_len);

	const char *host = instance->config.host;

	/* if @host isn't found in name */
	if (strchr(name, '@') + 1 != strstr(name, host))
	{
		fixed_name_len = name_len + strlen(host) + 1; /* +1 == +'@' */

		allocated_name = get_buffer(fixed_name_len + 1);
		memcpy(allocated_name, name, name_len);
		memcpy(allocated_name + name_len + 1, host, strlen(host));
		allocated_name[name_len] = '@';
		allocated_name[fixed_name_len] = 0;

		fixed_name = allocated_name;
	}

	id = get_id(tag, fixed_name, fixed_name_len);

	DEBUG("fixed name: %s, id: %lu\n", fixed_name, (unsigned long)(id));

	if (allocated_name != NULL)
	{
		free_buffer(allocated_name);
	}

	/* try to fallback to short name if there is error in resolving long name */   
	if (id == ACL_UNDEFINED_ID)
	{
		id = get_id(tag, name, name_len);
	}

	return id;
}

char* nss_reverse_resolve(acl_tag_t tag, void *id, void *instance_casted)
{
	const char *name = NULL;

	DEBUG("nss reverse resolve: id: %lu\n", id != NULL ? *(unsigned long *)(id) : ACL_UNDEFINED_ID);

	switch (tag)
	{
	case ACL_USER:
		{
			struct passwd *pwd = getpwuid(*(uid_t *)(id));
			if (pwd != NULL)
			{
				name = pwd->pw_name;
			}
		}
		break;
	case ACL_GROUP:
		{
			struct group *grp = getgrgid(*(gid_t *)(id));
			if (grp != NULL)
			{
				name = grp->gr_name;
			}
		}
		break;
	}

	if (name == NULL)
	{
		return NULL;
	}

	if (strchr(name, '@') == NULL)
	{
		return strdup(name);
	}

	/* remove @host from name, 
	in other words, make it local (for server) */
	char *host_pos = strchr(name, '@');
	size_t fixed_name_len = host_pos - name;
	
	char *fixed_name = malloc(fixed_name_len + 1);

	memcpy(fixed_name, name, fixed_name_len);
	fixed_name[fixed_name_len] = 0;

	return fixed_name;
}

#else
int acl_nss_resolve_c_empty_module_makes_suncc_mad = 0;
#endif /* ACL_AVAILABLE */


