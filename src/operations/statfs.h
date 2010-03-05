/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_STATFS_H
#define OPERATIONS_STATFS_H

/** statfs */

struct rfs_instance;
struct statvfs;
int _rfs_statfs(struct rfs_instance *instance, const char *path, struct statvfs *buf);

#endif /* OPERATIONS_STATFS_H */
