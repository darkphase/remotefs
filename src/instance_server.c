/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "config.h"
#include "instance.h"
#include "instance_server.h"
#include "list.h"

static void init_cleanup(struct rfsd_instance *instance)
{
	instance->cleanup.open_files = NULL;
}

static void init_keep_alive(struct rfsd_instance *instance)
{
	instance->keep_alive.lock = 0;
	instance->keep_alive.last_keep_alive = (time_t)(0);
}

static void init_server(struct rfsd_instance *instance)
{
	instance->server.hand_shaken = 0;
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
	add_to_list(&instance->config.listen_addresses, strdup(DEFAULT_IPV4_ADDRESS));
#ifdef WITH_IPV6
	add_to_list(&instance->config.listen_addresses, strdup(DEFAULT_IPV6_ADDRESS));
#endif
	instance->config.listen_port = DEFAULT_SERVER_PORT;
	instance->config.worker_uid = geteuid();
	instance->config.quiet = 0;
	
	instance->config.pid_file = strdup(DEFAULT_PID_FILE);
	instance->config.exports_file = strdup(DEFAULT_EXPORTS_FILE);
	instance->config.passwd_file = strdup(DEFAULT_PASSWD_FILE);
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
	
	init_rfsd_config(instance);
}

void release_rfsd_instance(struct rfsd_instance *instance)
{
	destroy_list(&(instance->config.listen_addresses));
	free(instance->config.pid_file);
	free(instance->config.exports_file);
	free(instance->config.passwd_file);
}
