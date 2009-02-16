/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_RFS_H
#define OPERATIONS_RFS_H

/** rfs internal operations */

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct rfs_instance;
struct answer;

void* rfs_init(struct rfs_instance *instance);
void rfs_destroy(struct rfs_instance *instance);

#ifdef WITH_SSL
int rfs_enablessl(struct rfs_instance *instance, unsigned show_errors);
#endif

int rfs_reconnect(struct rfs_instance *instance, unsigned int show_errors, unsigned int change_path);
void rfs_disconnect(struct rfs_instance *instance, int gently);

#ifdef WITH_EXPORTS_LIST
int rfs_list_exports(struct rfs_instance *instance);
#endif

int cleanup_badmsg(struct rfs_instance *instance, const struct answer *ans);
int check_connection(struct rfs_instance *instance);

int rfs_request_salt(struct rfs_instance *instance);
int rfs_auth(struct rfs_instance *instance, const char *user, const char *passwd);
int rfs_mount(struct rfs_instance *instance, const char *path);
int rfs_getexportopts(struct rfs_instance *instance, enum rfs_export_opts *opts);
int rfs_setsocktimeout(struct rfs_instance *instance, const int timeout);
int rfs_setsockbuffer(struct rfs_instance *instance, const int size);
int rfs_keep_alive(struct rfs_instance *instance);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* OPERATIONS_RFS_H */
