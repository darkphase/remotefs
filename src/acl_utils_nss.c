/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifdef WITH_ACL

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/acl.h>

#include "acl_utils.h"
#include "acl_utils_nss.h"
#include "buffer.h"
#include "config.h"
#include "id_lookup.h"
#include "instance_client.h"

uint32_t nss_resolve(uint16_t type, const char *name, size_t name_len, void *instance_casted)
{
	if (name == NULL 
	|| name_len == 0)
	{
		return ACL_UNDEFINED_ID;
	}

	struct rfs_instance *instance = (struct rfs_instance *)(instance_casted);

	DEBUG("resolving ACL name: %s\n", name);

	if (type == ACL_USER 
	&& strncmp(name, instance->config.auth_user, name_len) == 0)
	{
		return instance->client.my_uid;
	}

	const char *fixed_name = name;
	char *allocated_name = NULL;
	uint32_t id = ACL_UNDEFINED_ID;

	DEBUG("nss resolve: name: %s, name_len: %lu\n", name, (unsigned long)name_len);

	const char *host = instance->config.host;

	/* if @host isn't found in name */
	if (strchr(name, '@') + 1 != strstr(name, host))
	{
		size_t overall_len = name_len + strlen(host);

		allocated_name = get_buffer(overall_len + 1);
		memcpy(allocated_name, name, name_len);
		memcpy(allocated_name + name_len + 1, host, strlen(host));
		allocated_name[name_len] = '@';
		allocated_name[overall_len + 1] = 0;

		fixed_name = allocated_name;
	}

	DEBUG("fixed name: %s\n", fixed_name);

	switch (type)
	{
	case ACL_USER:
		{
		uid_t uid = get_uid(instance->id_lookup.uids, fixed_name);
		if (uid != (uid_t)(-1))
		{
			id = (uint32_t)uid;
		}
		}
		break;
	case ACL_GROUP:
		{
		gid_t gid = get_gid(instance->id_lookup.gids, fixed_name);
		if (gid != (gid_t)(-1))
		{
			id = (uint32_t)gid;
		}
		}
		break;
	}

	if (allocated_name != NULL)
	{
		free_buffer(allocated_name);
	}

	return id;
}

char* nss_reverse_resolve(uint16_t type, uint32_t id, void *instance_casted)
{
	struct rfs_instance *instance = (struct rfs_instance *)(instance_casted);
	const char *name = NULL;

	DEBUG("nss reverse resolve: id: %lu\n", (unsigned long)id);

	switch (type)
	{
	case ACL_USER:
		name = get_uid_name(instance->id_lookup.uids, (uid_t)id);
		break;
	case ACL_GROUP:
		name = get_gid_name(instance->id_lookup.gids, (gid_t)id);
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
int acl_utils_nss_c_empty_module_makes_suncc_mad = 0;
#endif /* WITH_ACL*/


