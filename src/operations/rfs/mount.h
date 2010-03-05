/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_RFS_MOUNT_H
#define OPERATIONS_RFS_MOUNT_H

/** rfs_mount */

struct rfs_instance;
int rfs_mount(struct rfs_instance *instance, const char *path);

#endif /* OPERATIONS_RFS_MOUNT_H */
