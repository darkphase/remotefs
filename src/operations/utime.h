/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_UTIME_H
#define OPERATIONS_UTIME_H

/** utime */

struct rfs_instance;
struct utimbuf;
int _rfs_utime(struct rfs_instance *instance, const char *path, struct utimbuf *buf);

#endif /* OPERATIONS_UTIME_H */
