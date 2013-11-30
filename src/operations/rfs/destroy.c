/*
remotefs file system
See the file AUTHORS for copyright information.

This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "../../options.h"
#include "../../attr_cache.h"
#include "../../config.h"
#include "../../id_lookup.h"
#include "../../instance_client.h"
#include "../../keep_alive_client.h"
#include "../../nss/server.h"
#include "../../resume/resume.h"
#include "../../sendrecv_client.h"
#include "disconnect.h"

void rfs_destroy(struct rfs_instance *instance)
{
	client_keep_alive_lock(instance);

#if defined RFSNSS_AVAILABLE
	if (is_nss_running(instance))
	{
		stop_nss_server(instance);
	}
#endif

	rfs_disconnect(instance, 1);

	kill_write_behind(instance);

	client_keep_alive_unlock(instance);

	instance->client.maintenance_please_die = 1;

	if (instance->client.maintenance_thread != 0)
	{
		pthread_join(instance->client.maintenance_thread, NULL);
	}

	client_keep_alive_destroy(instance);

	destroy_cache(&instance->attr_cache);
	destroy_resume_lists(&instance->resume.open_files, &instance->resume.locked_files);

	destroy_uids_lookup(&(instance->id_lookup.uids));
	destroy_gids_lookup(&(instance->id_lookup.gids));

#ifdef RFS_DEBUG
	dump_attr_stats(&instance->attr_cache);
	dump_sendrecv_stats(&instance->sendrecv);
#endif
}
