/*
remotefs file system
See the file AUTHORS for copyright information.

This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef INSTANCE_CLIENT_H
#define INSTANCE_CLIENT_H

/** remotefs client instances */

#include "attr_cache.h"
#include "config.h"
#include "exports.h"
#include "instance.h"
#include "operations/write.h"
#include "psemaphore.h"

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
	struct attr_cache attr_cache;

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
		int please_die;
		int last_ret;
		pthread_t write_behind_thread;
		rfs_sem_t write_behind_started;
		rfs_sem_t write_behind_sem;
		rfs_write_cache_block_t block1;
		rfs_write_cache_block_t block2;
		rfs_write_cache_block_t *current_block;
	} write_cache;

	/* sendrecv */
	rfs_sendrecv_info_t sendrecv;

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

	rfs_id_lookup_info_t id_lookup;

	rfs_config_t config;
};

#define DECLARE_RFS_INSTANCE(name) extern struct rfs_instance (name)
#define DEFINE_RFS_INSTANCE(name) struct rfs_instance (name) = { { 0 } }

/* initialise rfs instance */
void init_rfs_instance(struct rfs_instance *instance);

/* release memory used by rfs instance */
void release_rfs_instance(struct rfs_instance *instance);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* INSTANCE_CLIENT_H */
