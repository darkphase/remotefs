/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_UTIMENS_H
#define OPERATIONS_UTIMENS_H

/** utimens */

struct rfs_instance;
struct timespec;
int _rfs_utimens(struct rfs_instance *instance, const char *path, const struct timespec tv[2]);

#endif /* OPERATIONS_UTIMENS_H */
