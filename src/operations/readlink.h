/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_READLINK_H
#define OPERATIONS_READLINK_H

/** readlink */

#include <sys/types.h>

struct rfs_instance;
int _rfs_readlink(struct rfs_instance *instance, const char *path, char *buffer, size_t size);

#endif /* OPERATIONS_READLINK_H */
