/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_TRUNCATE_H
#define OPERATIONS_TRUNCATE_H

/** truncate */

#include <sys/types.h>

struct rfs_instance;
int _rfs_truncate(struct rfs_instance *instance, const char *path, off_t offset);

#endif /* OPERATIONS_TRUNCATE_H */
