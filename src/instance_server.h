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

#ifdef WITH_SSL
#include <openssl/ssl.h>
#endif

#include "config.h"
#include "psemaphore.h"
#include "instance.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct rfs_export;
#ifdef WITH_PAUSE
struct timeval;
#endif

struct rfsd_instance
{
	/* server variables */
	struct 
	{
		unsigned directory_mounted;
		struct rfs_export *mounted_export;
		char *auth_user;
		char *auth_passwd;
		char auth_salt[MAX_SALT_LEN + 1];
	} server;
	
	/* clean up after connection */
	struct
	{
		struct list *open_files;
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
		struct list *auths;
	} passwd;
	
	/* exports */
	struct 
	{
		struct list *list;
	} exports;
	
	/* sendrecv */
	struct sendrecv_info sendrecv;
	
	/* id lookup */
	struct id_lookup_info id_lookup;
	
#ifdef WITH_SSL
	/* ssl */
	struct ssl_info ssl;
#endif

#ifdef WITH_PAUSE
	struct
	{
		struct timeval last;
	} pause;
#endif

	/* server's config */
	struct rfsd_config config;
};

#define DEFINE_RFSD_INSTANCE(name) struct rfsd_instance (name) = { { 0 } }

/* initialise rfsd instance */
void init_rfsd_instance(struct rfsd_instance *instance);

/* release memory used by rfsd instance */
void release_rfsd_instance(struct rfsd_instance *instance);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* INSTANCE_SERVER_H */

