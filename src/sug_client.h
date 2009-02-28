/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef SUG_CLIENT_H
#define SUG_CLIENT_H

/** suggestions for remotefs client */

struct rfs_instance;

/** make suggestions for client */
void suggest_client(const struct rfs_instance *instance);

#endif /* SUG_CLIENT_H */

