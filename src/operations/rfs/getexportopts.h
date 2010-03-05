/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_RFS_GETEXPORTOPTS_H
#define OPERATIONS_RFS_GETEXPORTOPTS_H

/** rfs_getexportopts */

struct rfs_instance;
enum rfs_export_opts;
int rfs_getexportopts(struct rfs_instance *instance, enum rfs_export_opts *opts);

#endif /* OPERATIONS_RFS_GETEXPORTOPTS_H */
