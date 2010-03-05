/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_UNLINK_H
#define OPERATIONS_UNLINK_H

/** unlink */

struct rfs_instance;
int _rfs_unlink(struct rfs_instance *instance, const char *path);

#endif /* OPERATIONS_UNLINK_H */
