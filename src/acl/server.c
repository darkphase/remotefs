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

#include "server.h"
#include "../config.h"

#define RFS_XATTR_NAME_ACL_ACCESS  "system.posix_acl_access"
#define RFS_XATTR_NAME_ACL_DEFAULT "system.posix_acl_default"

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
	*acl = acl_get_file(path, acl_type);

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

