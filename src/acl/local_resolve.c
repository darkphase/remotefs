/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include "../options.h"

#ifdef ACL_OPERATIONS_AVAILABLE

#include <stdlib.h>

#include "local_resolve.h"
#include "../buffer.h"
#include "../config.h"
#include "../id_lookup_client.h"
#include "../instance_client.h"
#include "../names.h"

static uint32_t resolve_name(acl_tag_t tag, const char *name, const struct rfs_instance *instance)
{
	DEBUG("locally resolving name: %s\n", name);

	uint32_t id = ACL_UNDEFINED_ID;

	switch (tag)
	{
	case ACL_USER:
		id = (uint32_t)(lookup_user(instance, name));
		break;
	case ACL_GROUP:
		id = (uint32_t)(lookup_group(instance, name, NULL));
		break;
	}

	return id;
}

uint32_t local_resolve(acl_tag_t tag, const char *name, size_t name_len, void *instance_casted)
{
	const struct rfs_instance *instance = (const struct rfs_instance *)(instance_casted);

	char *dup_name = buffer_dup_str(name, name_len);
	if (dup_name == NULL)
	{
		return ACL_UNDEFINED_ID;
	}

	/* try to resolve name as remote if nss is enabled */ 
	if (instance->nss.use_nss != 0)
	{
		if (is_nss_name(dup_name) == 0)
		{
			char *nss_name = remote_nss_name(dup_name, instance);
			uint32_t id = resolve_name(tag, nss_name, instance);
			free(nss_name);

			if (id != ACL_UNDEFINED_ID)
			{
				free(dup_name);
				return id;
			}
		}
	}

	/* fallback to "short" name resolve if nss isn't enabled or long name 
	can't be resolved */
	uint32_t id = resolve_name(tag, dup_name, instance);
	
	free(dup_name);

	DEBUG("id: %lu\n", (unsigned long)id);
	return id;
}

static const char* resolve_id(acl_tag_t tag, const void *id, const struct rfs_instance *instance)
{
	const char *looked_up_name = NULL;

	switch (tag)
	{
	case ACL_USER:
		looked_up_name = lookup_uid(instance, *(uid_t *)(id));
		break;
	case ACL_GROUP:
		looked_up_name = lookup_gid(instance, *(gid_t *)(id), (uid_t)(-1));
		break;
	}

	return looked_up_name;
}

char* local_reverse_resolve(acl_tag_t tag, const void *id, void *instance_casted)
{
	DEBUG("locally reverse resolving id: %lu\n", id != NULL ? *(unsigned long *)(id) : ACL_UNDEFINED_ID);

	const char *name = resolve_id(tag, id, (const struct rfs_instance *)(instance_casted));
	return (name != NULL ? strdup(name) : NULL);
}

#else
int acl_local_resolve_c_empty_module = 0; /* avoid warning about empty module */
#endif /* ACL_OPERATIONS_AVAILABLE */

