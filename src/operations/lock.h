/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_LOCK_H
#define OPERATIONS_LOCK_H

/** lock */

#include <stdint.h>

struct flock;
struct rfs_instance;
int _rfs_lock(struct rfs_instance *instance, const char *path, uint64_t desc, int cmd, struct flock *fl);

#endif /* OPERATIONS_LOCK_H */
