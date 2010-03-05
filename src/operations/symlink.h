/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_SYMLINK_H
#define OPERATIONS_SYMLINK_H

/** symlink */

struct rfs_instance;
int _rfs_symlink(struct rfs_instance *instance, const char *path, const char *target);

#endif /* OPERATIONS_SYMLINK_H */
