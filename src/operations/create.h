/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_CREATE_H
#define OPERATIONS_CREATE_H

/** create */

#include <stdint.h>
#include <sys/types.h>

struct rfs_instance;
int _rfs_create(struct rfs_instance *instance, const char *path, mode_t mode, int flags, uint64_t *desc);

#endif /* OPERATIONS_CREATE_H */
