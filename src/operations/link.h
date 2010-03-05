/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_LINK_H
#define OPERATIONS_LINK_H

/** link */

struct rfs_instance;
int _rfs_link(struct rfs_instance *instance, const char *path, const char *target);

#endif /* OPERATIONS_LINK_H */
