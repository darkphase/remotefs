/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef LIST_EXPORTS_H
#define LIST_EXPORTS_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct rfs_instance;

#ifdef WITH_EXPORTS_LIST
int list_exports_main(struct rfs_instance *instance);
#endif

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* LIST_EXPORTS_H */

