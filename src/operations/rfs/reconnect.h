/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_RFS_RECONNECT_H
#define OPERATIONS_RFS_RECONNECT_H

/** rfs_reconnect */

struct rfs_instance;
int rfs_reconnect(struct rfs_instance *instance, unsigned int show_errors, unsigned int change_path);

#endif /* OPERATIONS_RFS_RECONNECT_H */
