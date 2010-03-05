/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include "../options.h"

#ifdef ACL_OPERATIONS_AVAILABLE

#ifndef OPERATIONS_ACL_GETXATTR_H
#define OPERATIONS_ACL_GETXATTR_H

/** getxattr */

#include <sys/types.h>

struct rfs_instance;
int _rfs_getxattr(struct rfs_instance *instance, const char *path, const char *name, char *value, size_t size);

#endif /* OPERATIONS_ACL_GETXATTR_H */
#endif /* ACL_OPERATIONS_AVAILABLE */
