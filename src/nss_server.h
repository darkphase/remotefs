/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifdef WITH_UGO

#ifndef NSS_SERVER_H
#define NSS_SERVER_H

/** nss server */

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct rfs_instance;

int start_nss_server(struct rfs_instance *instance);
int stop_nss_server(struct rfs_instance *instance);
unsigned is_nss_running(struct rfs_instance *instance);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* NSS_SERVER_H */

#endif /* WITH_UGO */

