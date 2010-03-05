/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_RFS_DISCONNECT_H
#define OPERATIONS_RFS_DISCONNECT_H

/** rfs_disconnect */

struct rfs_instance;
void  rfs_disconnect(struct rfs_instance *instance, int gently);

#endif /* OPERATIONS_RFS_DISCONNECT_H */
