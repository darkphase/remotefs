/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_RFS_AUTH_H
#define OPERATIONS_RFS_AUTH_H

/** rfs_auth */

struct rfs_instance;
int rfs_auth(struct rfs_instance *instance, const char *user, const char *passwd);

#endif /* OPERATIONS_RFS_AUTH_H */
