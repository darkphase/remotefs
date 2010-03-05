/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_OPEN_H
#define OPERATIONS_OPEN_H

/** open */

#include <stdint.h>

struct rfs_instance;
int _rfs_open(struct rfs_instance *instance, const char *path, int flags, uint64_t *desc);

#endif /* OPERATIONS_OPEN_H */
