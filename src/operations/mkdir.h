/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_MKDIR_H
#define OPERATIONS_MKDIR_H

/** mkdir */

#include <sys/types.h>

struct rfs_instance;
int _rfs_mkdir(struct rfs_instance *instance, const char *path, mode_t mode);

#endif /* OPERATIONS_MKDIR_H */
