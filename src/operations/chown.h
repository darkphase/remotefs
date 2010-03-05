/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_CHOWN_H
#define OPERATIONS_CHOWN_H

/** chown */

#include <sys/types.h>

struct rfs_instance;
int _rfs_chown(struct rfs_instance *instance, const char *path, uid_t uid, gid_t gid);

#endif /* OPERATIONS_H */
