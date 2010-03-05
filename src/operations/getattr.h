/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_GETATTR_H
#define OPERATIONS_GETATTR_H

/** getattr */

struct rfs_instance;
struct stat;
int _rfs_getattr(struct rfs_instance *instance, const char *path, struct stat *stbuf);

#endif /* OPERATIONS_GETATTR_H */
