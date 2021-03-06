/*
remotefs file system
See the file AUTHORS for copyright information.

This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef INSTANCE_SERVER_H
#define INSTANCE_SERVER_H

/** remotefs instances */

#include <sys/types.h>
#include <pthread.h>

#include "config.h"
#include "psemaphore.h"
#include "instance.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct rfs_export;
struct timeval;

struct rfsd_instance
{
	/* server variables */
	struct
	{
		unsigned hand_shaken;
		unsigned directory_mounted;
		struct rfs_export *mounted_export;
		char *auth_user;
		char *auth_passwd;
		char auth_salt[MAX_SALT_LEN + 1];
	} server;

	/* clean up after connection */
	struct
	{
		struct rfs_list *open_files;
	} cleanup;

	/* keep-alive */
	struct
	{
		unsigned int lock;
		time_t last_keep_alive;
	} keep_alive;

	/* passwd db */
	struct
	{
		struct rfs_list *auths;
	} passwd;

	/* exports */
	struct
	{
		struct rfs_list *list;
	} exports;

	/* sendrecv */
	rfs_sendrecv_info_t sendrecv;

	/* id lookup */
	rfs_id_lookup_info_t id_lookup;

	/* server's config */
	rfsd_config_t config;
};

#define DECLARE_RFSD_INSTANCE(name) extern struct rfsd_instance (name)
#define DEFINE_RFSD_INSTANCE(name) struct rfsd_instance (name) = { { 0 } }

/* initialise rfsd instance */
void init_rfsd_instance(struct rfsd_instance *instance);

/* release memory used by rfsd instance */
void release_rfsd_instance(struct rfsd_instance *instance);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* INSTANCE_SERVER_H */
