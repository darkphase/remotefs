/*
remotefs file system
See the file AUTHORS for copyright information.

This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "buffer.h"
#include "config.h"
#include "instance_client.h"

static void init_client(struct rfs_instance *instance)
{
	instance->client.maintenance_thread = 0;
	instance->client.maintenance_please_die = 0;
	memset(instance->client.auth_salt, 0, sizeof(instance->client.auth_salt));
	instance->client.export_opts = OPT_NONE;
	instance->client.my_uid = getuid();
	instance->client.my_gid = getgid();
	instance->client.my_pid = getpid();
}

static void init_attr_cache(struct attr_cache *cache)
{
	/* attrs cache */
	cache->cache = NULL;
	cache->last_time_checked = (time_t)(0);
	cache->number_of_entries = 0;
#ifdef RFS_DEBUG
	cache->hits = 0;
	cache->misses = 0;
	cache->max_number_of_entries = 0;
#endif
}

static void init_resume(struct rfs_instance *instance)
{
	instance->resume.open_files = NULL;
	instance->resume.locked_files = NULL;
}

static void init_write_cache(struct rfs_instance *instance)
{
	/* write cache */
	instance->write_cache.max_cache_size = DEFAULT_RW_CACHE_SIZE;
	instance->write_cache.cache = NULL;
	instance->write_cache.write_behind_thread = 0;
	memset(&instance->write_cache.write_behind_request, 0, sizeof(instance->write_cache.write_behind_request));
}

static void init_nss(struct rfs_instance *instance)
{
	/* nss */
	instance->nss.socket = -1;
	instance->nss.users_storage = NULL;
	instance->nss.groups_storage = NULL;
	instance->nss.server_thread = 0;
	instance->nss.use_nss = 1;
	rfs_sem_init(&instance->nss.thread_ready, 0, 0);
}

static void init_rfs_config(struct rfs_instance *instance)
{
	instance->config.server_port = DEFAULT_SERVER_PORT;
	instance->config.quiet = 0;
	instance->config.use_write_cache = 1;
	instance->config.transform_symlinks = 0;
	instance->config.allow_other = 0;
	instance->config.set_fsname = 1;
}

void init_rfs_instance(struct rfs_instance *instance)
{
	init_client(instance);
	init_attr_cache(&instance->attr_cache);
	init_id_lookup(&instance->id_lookup);
	init_resume(instance);
	init_write_cache(instance);
	init_sendrecv(&instance->sendrecv);
	init_nss(instance);

	init_rfs_config(instance);
}

void release_rfs_instance(struct rfs_instance *instance)
{
}
