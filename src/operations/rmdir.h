/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_RMDIR_H
#define OPERATIONS_RMDIR_H

/** rmdir */

struct rfs_instance;
int _rfs_rmdir(struct rfs_instance *instance, const char *path);

#endif /* OPERATIONS_RMDIR_H */
