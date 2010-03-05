/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_CHMOD_H
#define OPERATIONS_CHMOD_H

/** chmod */

#include <sys/types.h>

struct rfs_instance;
int _rfs_chmod(struct rfs_instance *instance, const char *path, mode_t mode);

#endif /* OPERATIONS_CHMOD_H */
