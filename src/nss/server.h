/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef NSS_SERVER_H
#define NSS_SERVER_H

#ifdef WITH_UGO

/** nss server */

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct rfs_instance;

int init_nss_server(struct rfs_instance *instance, unsigned show_errors);
int start_nss_server(struct rfs_instance *instance);
int stop_nss_server(struct rfs_instance *instance);
unsigned is_nss_running(struct rfs_instance *instance);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* WITH_UGO */
#endif /* NSS_SERVER_H */
