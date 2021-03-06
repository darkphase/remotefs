/*
remotefs file system
See the file AUTHORS for copyright information.

This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include "../options.h"

#ifdef ACL_AVAILABLE

#include <stdlib.h>
#include <string.h>

#include "id_lookup_resolve.h"
#include "../buffer.h"
#include "../config.h"
#include "../id_lookup.h"
#include "../instance.h"

#define RFS_XATTR_NAME_ACL_ACCESS  "system.posix_acl_access"
#define RFS_XATTR_NAME_ACL_DEFAULT "system.posix_acl_default"

char* id_lookup_reverse_resolve(acl_tag_t tag, const void *id, void *lookup_casted)
{
	rfs_id_lookup_info_t *lookup = (rfs_id_lookup_info_t *)(lookup_casted);

	switch (tag)
	{
	case ACL_USER:
		{
		DEBUG("resolving username with id: %d\n", *(uid_t *)(id));

		const char *username = get_uid_name(lookup->uids, *(uid_t *)(id));

		if (username == NULL)
		{
			return NULL;
		}

		return strdup(username);
		}
	case ACL_GROUP:
		{
		DEBUG("resolving groupname with id: %d\n", *(uid_t *)(id));

		const char *groupname = get_gid_name(lookup->gids, *(gid_t *)(id));

		if (groupname == NULL)
		{
			return NULL;
		}

		return strdup(groupname);
		}
	}

	return NULL;
}

uint32_t id_lookup_resolve(acl_tag_t tag, const char *name, size_t name_len, void *lookup_casted)
{
	rfs_id_lookup_info_t *lookup = (rfs_id_lookup_info_t *)(lookup_casted);

	switch (tag)
	{
	case ACL_USER:
		{
		char *username = malloc(name_len + 1);
		memcpy(username, name, name_len);
		username[name_len] = 0;

		DEBUG("resolving username: %s\n", username);

		uid_t uid = get_uid(lookup->uids, username);

		free(username);

		if (uid == (uid_t)-1)
		{
			return ACL_UNDEFINED_ID;
		}

		return (uint32_t)(uid);
		}
	case ACL_GROUP:
		{
		char *groupname = malloc(name_len + 1);
		memcpy(groupname, name, name_len);
		groupname[name_len] = 0;

		DEBUG("resolving groupname: %s\n", groupname);

		gid_t gid = get_gid(lookup->gids, groupname);

		free(groupname);

		if (gid == (gid_t)-1)
		{
			return ACL_UNDEFINED_ID;
		}

		return (uint32_t)(gid);
		}
	}

	return ACL_UNDEFINED_ID;
}

#else
int acl_id_lookup_resolve_c_empty_module = 0;
#endif /* ACL_AVAILABLE */

