/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_MKNOD_H
#define OPERATIONS_MKNOD_H

/** mknod */

#include <sys/types.h>

struct rfs_instance;
int _rfs_mknod(struct rfs_instance *instance, const char *path, mode_t mode, dev_t dev);

#endif /* OPERATIONS_MKNOD_H */
