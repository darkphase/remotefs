/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_RENAME_H
#define OPERATIONS_RENAME_H

/** rename */

struct rfs_instance;
int _rfs_rename(struct rfs_instance *instance, const char *path, const char *new_path);

#endif /* OPERATIONS_RENAME_H */
