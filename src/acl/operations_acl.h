/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include "../options.h"

#ifdef ACL_OPERATIONS_AVAILABLE

#ifndef OPERATIONS_ACL_H
#define OPERATIONS_ACL_H

/** ACL specific rfs operations */

#include <sys/types.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct rfs_instance;

int _rfs_getxattr(struct rfs_instance *instance, const char *path, const char *name, char *value, size_t size);
int _rfs_setxattr(struct rfs_instance *instance, const char *path, const char *name, const char *value, size_t size, int flags);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* OPERATIONS_ACL_H */
#endif /* ACL_OPERATIONS_AVAILABLE */
