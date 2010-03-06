/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_LISTEXPORTS_H
#define OPERATIONS_LISTEXPORTS_H

/** list exports */

#ifdef WITH_EXPORTS_LIST
struct rfs_instance;
int rfs_listexports(struct rfs_instance *instance);
#endif

#endif /* OPERATIONS_LISTEXPORTS_H */
