/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef INSTANCE_CLIENT_H
#define INSTANCE_CLIENT_H

/** remotefs client instances */

#include <sys/types.h>
#include <pthread.h>

#ifdef WITH_SSL
#include <openssl/ssl.h>
#endif

#include "config.h"
#include "pt_semaphore.h"
#include "instance.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct rfs_instance
{
	/* client variables */
	struct 
	{
		pthread_t maintenance_thread;
		unsigned maintenance_please_die;
		
		char auth_salt[MAX_SALT_LEN + 1];
		enum rfs_export_opts export_opts;
		
		uid_t my_uid;
		uid_t my_gid;
	} client;
	
	/* attributes cache */
	struct
	{
		void *cache;
		time_t last_time_checked;
#ifdef RFS_DEBUG
		unsigned long cache_hits;
		unsigned long cache_misses;
#endif
	} attr_cache;
	
	/* keep-alive */
	struct 
	{
		pthread_mutex_t mutex;
	} keep_alive;
	
	/* resume */
	struct
	{
		struct list* open_files;
		struct list* locked_files;
	} resume;
	
	/* read cache */
	struct
	{
		size_t max_cache_size;
		struct list *cache;
		struct prefetch_request prefetch_request;
		pthread_t prefetch_thread;
		rfs_sem_t prefetch_sem;
		rfs_sem_t prefetch_started;
	} read_cache;
	
	/* write cache */
	struct
	{
		size_t max_cache_size;
		struct list *cache;
		pthread_t write_behind_thread;
		rfs_sem_t write_behind_started;
		rfs_sem_t write_behind_sem;
		struct write_behind_request write_behind_request;
	} write_cache;
	
	/* sendrecv */
	struct sendrecv_info sendrecv;
	
	/* id lookup */
	struct id_lookup_info id_lookup;
	
#ifdef WITH_SSL
	/* ssl */
	struct ssl_info ssl;
#endif

	/* nss server */
	struct
	{
		int socket;
		struct list *users_storage;
		struct list *groups_storage;
		pthread_t server_thread;
	} nss;

	struct rfs_config config;
};

#define DEFINE_RFS_INSTANCE(name) struct rfs_instance (name) = { { 0 } }

/* initialise rfs instance */
void init_rfs_instance(struct rfs_instance *instance);

/* release memory used by rfs instance */
void release_rfs_instance(struct rfs_instance *instance);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* INSTANCE_CLIENT_H */

