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

static void init_sendrecv(struct sendrecv_info *sendrecv)
{
	sendrecv->socket = -1;
#ifdef WITH_SSL
	sendrecv->ssl_socket = NULL;
#endif
	sendrecv->oob_received = 0;
	sendrecv->connection_lost = 1; /* yes, it should be set to 0 on connect or accept */
#ifdef RFS_DEBUG
	sendrecv->bytes_sent = 0;
	sendrecv->bytes_recv = 0;
	sendrecv->send_susecs_used = (suseconds_t)0;
	sendrecv->recv_susecs_used = (suseconds_t)0;
#endif
}

#ifdef WITH_SSL
static void init_ssl(struct ssl_info *ssl)
{
	ssl->ctx = NULL;
	ssl->last_error = NULL;
}
#endif

static void init_id_lookup(struct id_lookup_info *id_lookup)
{
	id_lookup->uids = NULL;
	id_lookup->gids = NULL;
}

static void init_cleanup(struct rfsd_instance *instance)
{
	instance->cleanup.open_files = NULL;
	instance->cleanup.locked_files = NULL;
}

static void init_keep_alive(struct rfsd_instance *instance)
{
	instance->keep_alive.lock = 0;
	instance->keep_alive.last_keep_alive = (time_t)(0);
}

static void init_server(struct rfsd_instance *instance)
{
	instance->server.directory_mounted = 0;
	instance->server.mounted_export = NULL;
	instance->server.auth_user = NULL;
	instance->server.auth_passwd = NULL;
	memset(instance->server.auth_salt, 0, sizeof(instance->server.auth_salt));
}

static void init_passwd(struct rfsd_instance *instance)
{
	instance->passwd.auths = NULL;
}

static void init_exports(struct rfsd_instance *instance)
{
	instance->exports.list = NULL;
}

static void init_rfsd_config(struct rfsd_instance *instance)
{
#ifndef WITH_IPV6
	instance->config.listen_address = strdup("0.0.0.0");
#else
	instance->config.listen_address = strdup("::");
#endif
	instance->config.listen_port = DEFAULT_SERVER_PORT;
	instance->config.worker_uid = geteuid();
	instance->config.worker_gid = getegid();
	instance->config.quiet = 0;
	
	instance->config.pid_file = strdup(DEFAULT_PID_FILE);
	instance->config.exports_file = strdup(DEFAULT_EXPORTS_FILE);
	instance->config.passwd_file = strdup(DEFAULT_PASSWD_FILE);

#ifdef WITH_SSL
	instance->config.ssl_ciphers = RFS_DEFAULT_CIPHERS;
	instance->config.ssl_key_file = DEFAULT_SSL_KEY_FILE;
	instance->config.ssl_cert_file = DEFAULT_SSL_CERT_FILE;
#endif /* WITH_SSL */
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

void init_rfsd_instance(struct rfsd_instance *instance)
{
	init_cleanup(instance);
	init_keep_alive(instance);
	init_server(instance);
	init_passwd(instance);
	init_exports(instance);
	init_id_lookup(&instance->id_lookup);
	init_sendrecv(&instance->sendrecv);
#ifdef WITH_SSL
	init_ssl(&instance->ssl);
#endif
	
	init_rfsd_config(instance);
}

void release_rfsd_instance(struct rfsd_instance *instance)
{
	free(instance->config.listen_address);
	free(instance->config.pid_file);
	free(instance->config.exports_file);
	free(instance->config.passwd_file);
}

