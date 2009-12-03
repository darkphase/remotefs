/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef RESUME_CLIENT_H
#define RESUME_CLIENT_H

struct rfs_instance;

/** call necessary _rfs_open(), _rfs_lock() according to resume lists 
to restore files state after connection is restored 
\return 0 on success */
int resume_files(struct rfs_instance *instance);

#endif /* RESUME_CLIENT_H */

