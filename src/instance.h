/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef INSTANCE_H
#define INSTANCE_H

/** remotefs instances */

#include <sys/types.h>
#include <pthread.h>

#ifdef WITH_SSL
#include <openssl/ssl.h>
#endif

#include "config.h"
#include "rfs_semaphore.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct list;

struct sendrecv_info
{
	int socket;
	unsigned connection_lost;
	unsigned oob_received; /* used by client only */
#ifdef WITH_SSL
	int ssl_enabled;
	SSL *ssl_socket;
#endif
#ifdef RFS_DEBUG
	unsigned long bytes_sent;
	unsigned long bytes_recv;
	suseconds_t send_susecs_used;
	suseconds_t recv_susecs_used;
#endif
};

#ifdef WITH_SSL
struct ssl_info
{
	SSL_CTX *ctx;
	char *last_error;
};
#endif

struct id_lookup_info
{
	struct list *uids;
	struct list *gids;
};

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

	struct rfs_config config;
};

#define DEFINE_RFS_INSTANCE(name) struct rfs_instance (name) = { { 0 } }

struct rfs_export;

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
		struct list *locked_files;
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

	/* server's config */
	struct rfsd_config config;
};

#define DEFINE_RFSD_INSTANCE(name) struct rfsd_instance (name) = { { 0 } }

/* initialise rfs instance */
void init_rfs_instance(struct rfs_instance *instance);

/* release memory used by rfs instance */
void release_rfs_instance(struct rfs_instance *instance);

/* initialise rfsd instance */
void init_rfsd_instance(struct rfsd_instance *instance);

/* release memory used by rfsd instance */
void release_rfsd_instance(struct rfsd_instance *instance);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* INSTANCE_H */

