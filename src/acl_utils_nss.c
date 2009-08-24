/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include "options.h"

#ifdef ACL_AVAILABLE

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

static uint32_t get_id(uint16_t type, const struct rfs_instance *instance, const char *name, size_t name_len)
{
	char *dup_name = get_buffer(name_len + 1);
	memcpy(dup_name, name, name_len);
	dup_name[name_len] = 0;
	
	DEBUG("getting name: %s\n", dup_name);

	switch (type)
	{
	case ACL_USER:
		{
		uid_t uid = get_uid(instance->id_lookup.uids, dup_name);
		free_buffer(dup_name);

		if (uid != (uid_t)(-1))
		{
			return (uint32_t)uid;
		}
		}
		break;
	case ACL_GROUP:
		{
		gid_t gid = get_gid(instance->id_lookup.gids, dup_name);
		free_buffer(dup_name);

		if (gid != (gid_t)(-1))
		{
			return (uint32_t)gid;
		}
		}
		break;
	}

	return (uint32_t)(-1);
}

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
		allocated_name[fixed_name_len + 1] = 0;

		fixed_name = allocated_name;
	}

	id = get_id(type, instance, fixed_name, fixed_name_len);

	DEBUG("fixed name: %s, id: %lu\n", fixed_name, (unsigned long)(id));

	if (allocated_name != NULL)
	{
		free_buffer(allocated_name);
	}

	if (id == (uint32_t)(-1))
	{
		id = get_id(type, instance, name, name_len);
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
#endif /* ACL_AVAILABLE */


