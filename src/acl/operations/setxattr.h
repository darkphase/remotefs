/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include "../../options.h"

#ifdef ACL_OPERATIONS_AVAILABLE

#ifndef OPERATIONS_ACL_SETXATTR_H
#define OPERATIONS_ACL_SETXATTR_H

/** setxattr */

#include <sys/types.h>

struct rfs_instance;
int _rfs_setxattr(struct rfs_instance *instance, const char *path, const char *name, const char *value, size_t size, int flags);

#endif /* OPERATIONS_ACL_SETXATTR_H */
#endif /* ACL_OPERATIONS_AVAILABLE */
