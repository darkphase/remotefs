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
	instance->nss.use_nss = 1;
	rfs_sem_init(&instance->nss.thread_ready, 0, 0);
}

static void init_rfs_config(struct rfs_instance *instance)
{
	instance->config.server_port = DEFAULT_SERVER_PORT;
	instance->config.quiet = 0;
	instance->config.socket_timeout = -1;
	instance->config.socket_buffer = -1;
#ifdef WITH_SSL
	instance->config.enable_ssl = 0;
	instance->config.ssl_ciphers = strdup(RFS_DEFAULT_CIPHERS);

#ifndef RFS_DEBUG
	char *home_dir = getenv("HOME");
#else
	char *home_dir = ".";
#endif
	char *key = NULL;
	char *cert = NULL;

	if (home_dir != NULL)
	{
		size_t key_path_size = strlen(home_dir) + strlen(DEFAULT_CLIENT_KEY_FILE) + 2; /* 2 == '/' + '\0' */
		key = get_buffer(key_path_size);
		if (key != NULL)
		{
			snprintf(key, key_path_size, "%s/%s", home_dir, DEFAULT_CLIENT_KEY_FILE);
		}
	
		size_t cert_path_size = strlen(home_dir) + strlen(DEFAULT_CLIENT_CERT_FILE) + 2;
		cert = get_buffer(cert_path_size);
		if (cert != NULL)
		{
			snprintf(cert, cert_path_size, "%s/%s", home_dir, DEFAULT_CLIENT_CERT_FILE);
		}
	}

	instance->config.ssl_key_file = key;
	instance->config.ssl_cert_file = cert;
#endif /* WITH_SSL */
	
	instance->config.use_write_cache = 1;
	instance->config.transform_symlinks = 0;
}

void init_rfs_instance(struct rfs_instance *instance)
{
	init_client(instance);
	init_attr_cache(instance);
	init_resume(instance);
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
#ifdef WITH_SSL
	free(instance->config.ssl_ciphers);
	free(instance->config.ssl_key_file);
	free(instance->config.ssl_cert_file);
#endif
}

