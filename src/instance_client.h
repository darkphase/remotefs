/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef INSTANCE_CLIENT_H
#define INSTANCE_CLIENT_H

/** remotefs client instances */

#include "config.h"
#include "psemaphore.h"
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
		gid_t my_gid;
		pid_t my_pid;
	} client;
	
	/* attributes cache */
	struct
	{
		void *cache;
		time_t last_time_checked;
		unsigned long number_of_entries;
#ifdef RFS_DEBUG
		unsigned long cache_hits;
		unsigned long cache_misses;
		unsigned max_number_of_entries;
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
		unsigned use_nss;
		rfs_sem_t thread_ready;
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

