/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_RELEASE_H
#define OPERATIONS_RELEASE_H

/** release */

#include <stdint.h>

struct rfs_instance;
int _rfs_release(struct rfs_instance *instance, const char *path, uint64_t desc);

#endif /* OPERATIONS_RELEASE_H */
