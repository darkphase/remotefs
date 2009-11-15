/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include "../options.h"

#ifdef ACL_AVAILABLE

#include <errno.h>
#include <string.h>

#include "acl_utils.h"
#include "../buffer.h"
#include "../config.h"
#include "../id_lookup.h"
#include "../instance.h"

#define RFS_XATTR_NAME_ACL_ACCESS  "system.posix_acl_access"
#define RFS_XATTR_NAME_ACL_DEFAULT "system.posix_acl_defult"

char* id_lookup_reverse_resolve(acl_tag_t tag, void *id, void *lookup_casted)
{
	struct id_lookup_info *lookup = (struct id_lookup_info *)(lookup_casted);

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
	struct id_lookup_info *lookup = (struct id_lookup_info *)(lookup_casted);

	switch (tag)
	{
	case ACL_USER:
		{
		char *username = get_buffer(name_len + 1);
		memcpy(username, name, name_len);
		username[name_len] = 0;
	
		DEBUG("resolving username: %s\n", username);
		
		uid_t uid = get_uid(lookup->uids, username);
				
		free_buffer(username);
				
		if (uid == (uid_t)-1)
		{
			return ACL_UNDEFINED_ID;
		}

		return (uint32_t)(uid);
		}
	case ACL_GROUP:
		{
		char *groupname = get_buffer(name_len + 1);
		memcpy(groupname, name, name_len);
		groupname[name_len] = 0;
				
		DEBUG("resolving groupname: %s\n", groupname);

		gid_t gid = get_gid(lookup->gids, groupname);
				
		free_buffer(groupname);
				
		if (gid == (gid_t)-1)
		{
			return ACL_UNDEFINED_ID;
		}
		
		return (uint32_t)(gid);
		}
	}

	return ACL_UNDEFINED_ID;
}

int rfs_get_file_acl(const char *path, const char *acl_name, acl_t *acl)
{
	DEBUG("getting ACL record (%s) from %s\n", acl_name, path);

	acl_type_t acl_type = 0;

	if (strcmp(acl_name, RFS_XATTR_NAME_ACL_ACCESS) == 0)
	{
		acl_type = ACL_TYPE_ACCESS;
	}
	else if (strcmp(acl_name, RFS_XATTR_NAME_ACL_DEFAULT) == 0)
	{
		acl_type = ACL_TYPE_DEFAULT;
	}

	if (acl_type != ACL_TYPE_ACCESS 
	&& acl_type != ACL_TYPE_DEFAULT)
	{
		return -ENOTSUP;
	}

	errno = 0;
	*acl = acl_get_file(path, ACL_TYPE_ACCESS);

	if (acl == NULL)
	{
		return -errno;
	}

	return 0;
}

int rfs_set_file_acl(const char *path, const char *acl_name, const acl_t acl)
{
	DEBUG("setting ACL (%s) to %s\n", acl_name, path);

	acl_type_t acl_type = 0;

	if (strcmp(acl_name, RFS_XATTR_NAME_ACL_ACCESS) == 0)
	{
		acl_type = ACL_TYPE_ACCESS;
	}
	else if (strcmp(acl_name, RFS_XATTR_NAME_ACL_DEFAULT) == 0)
	{
		acl_type = ACL_TYPE_DEFAULT;
	}

	if (acl_type != ACL_TYPE_ACCESS 
	&& acl_type != ACL_TYPE_DEFAULT)
	{
		return -ENOTSUP;
	}

	errno = 0;
	if (acl_set_file(path, acl_type, acl) != 0)
	{
		return -errno;
	}

	return 0;
}

#else
int acl_utils_server_c_empty_module = 0;
#endif /* ACL_AVAILABLE */

