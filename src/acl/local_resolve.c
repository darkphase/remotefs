/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include "../options.h"

#ifdef ACL_OPERATIONS_AVAILABLE

#include "local_resolve.h"
#include "../buffer.h"
#include "../config.h"
#include "../id_lookup_client.h"
#include "../instance_client.h"

uint32_t local_resolve(acl_tag_t tag, const char *name, size_t name_len, void *instance_casted)
{
	const struct rfs_instance *instance = (const struct rfs_instance *)(instance_casted);

	char *dup_name = buffer_dup_str(name, name_len);
	if (dup_name == NULL)
	{
		return ACL_UNDEFINED_ID;
	}

	DEBUG("locally resolving name: %s\n", dup_name);

	uint32_t id = ACL_UNDEFINED_ID;

	switch (tag)
	{
	case ACL_USER:
		id = (uint32_t)(lookup_user(instance, dup_name));
		break;
	case ACL_GROUP:
		id = (uint32_t)(lookup_group(instance, dup_name, NULL));
		break;
	}

	free_buffer(dup_name);

	DEBUG("id: %lu\n", (unsigned long)id);
	return id;
}

char* local_reverse_resolve(acl_tag_t tag, void *id, void *instance_casted)
{
	const struct rfs_instance *instance = (const struct rfs_instance *)(instance_casted);

	DEBUG("locally reverse resolving id: %lu\n", id != NULL ? *(unsigned long *)(id) : ACL_UNDEFINED_ID);

	char *name = NULL;
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

	if (looked_up_name != NULL)
	{
		name = strdup(looked_up_name);
	}

	return name;
}

#else
int acl_local_resolve_c_empty_module = 0; /* avoid warning about empty module */
#endif /* ACL_OPERATIONS_AVAILABLE */

