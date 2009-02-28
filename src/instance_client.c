/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <string.h>
#include <unistd.h>
#if defined FREEBSD || defined DARWIN
#include <stdlib.h>
#else
#include <malloc.h>
#endif

#include "config.h"
#include "instance.h"
#include "instance_client.h"
#include "sendrecv.h"
#include "pt_semaphore.h"

static void init_client(struct rfs_instance *instance)
{
	instance->client.maintenance_thread = 0;
	instance->client.maintenance_please_die = 0;
	memset(instance->client.auth_salt, 0, sizeof(instance->client.auth_salt));
	instance->client.export_opts = OPT_NONE;
	instance->client.my_uid = (uid_t)-1;
	instance->client.my_gid = (gid_t)-1;
}

static void init_attr_cache(struct rfs_instance *instance)
{
	/* attrs cache */
	instance->attr_cache.cache = NULL;
	instance->attr_cache.last_time_checked = (time_t)(0);
#ifdef RFS_DEBUG
	instance->attr_cache.cache_hits = 0;
	instance->attr_cache.cache_misses = 0;
#endif
}

static void init_resume(struct rfs_instance *instance)
{
	instance->resume.open_files = NULL;
	instance->resume.locked_files = NULL;
}

static void init_read_cache(struct rfs_instance *instance)
{
	/* read cache */
	instance->read_cache.max_cache_size = DEFAULT_RW_CACHE_SIZE;
	instance->read_cache.cache = NULL;
	instance->read_cache.prefetch_thread = 0;
}

static void init_write_cache(struct rfs_instance *instance)
{
	/* write cache */
	instance->write_cache.max_cache_size = DEFAULT_RW_CACHE_SIZE;
	instance->write_cache.cache = NULL;
	instance->write_cache.write_behind_thread = 0;
}

static void init_nss(struct rfs_instance *instance)
{
	/* nss */
	instance->nss.socket = -1;
	instance->nss.users_storage = NULL;
	instance->nss.groups_storage = NULL;
	instance->nss.server_thread = 0;
}

static void init_rfs_config(struct rfs_instance *instance)
{
	instance->config.server_port = DEFAULT_SERVER_PORT;
	instance->config.use_read_cache = 1;
	instance->config.use_write_cache = 1;
	instance->config.use_read_write_cache = 1;
	instance->config.quiet = 0;
	instance->config.socket_timeout = -1;
	instance->config.socket_buffer = -1;
#ifdef WITH_SSL
	instance->config.enable_ssl = 0;
	instance->config.ssl_ciphers = RFS_DEFAULT_CIPHERS;
	instance->config.ssl_key_file = DEFAULT_SSL_KEY_FILE;
	instance->config.ssl_cert_file = DEFAULT_SSL_CERT_FILE;
#endif /* WITH_SSL */
}

void init_rfs_instance(struct rfs_instance *instance)
{
	init_client(instance);
	init_attr_cache(instance);
	init_resume(instance);
	init_read_cache(instance);
	init_write_cache(instance);
	init_sendrecv(&instance->sendrecv);
	init_id_lookup(&instance->id_lookup);
#ifdef WITH_SSL
	init_ssl(&instance->ssl);
#endif
	init_nss(instance);

	init_rfs_config(instance);
}

void release_rfs_instance(struct rfs_instance *instance)
{
	/* nothing to do yet */
}

