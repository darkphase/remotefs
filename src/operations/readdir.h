/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_READDIR_H
#define OPERATIONS_READDIR_H

/** readdir */

typedef int(*rfs_readdir_callback_t)(const char *, void *);

struct rfs_instance;
int _rfs_readdir(struct rfs_instance *instance, const char *path, const rfs_readdir_callback_t callback, void *callback_data);

#endif /* OPERATIONS_READDIR_H */
