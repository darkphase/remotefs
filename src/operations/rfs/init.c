/*
remotefs file system
See the file AUTHORS for copyright information.

This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../../options.h"
#include "../../config.h"
#include "../../id_lookup.h"
#include "../../instance_client.h"
#include "../../keep_alive_client.h"
#include "../../nss/server.h"
#include "../../scheduling.h"
#include "../../sendrecv_client.h"
#include "../write.h"
#include "../utils.h"
#include "keepalive.h"

static void* maintenance(void *void_instance)
{
	struct rfs_instance *instance = (struct rfs_instance *)(void_instance);

	unsigned keep_alive_slept = 0;
	unsigned attr_cache_slept = 0;
	unsigned shorter_sleep = 1; /* secs */

	while (instance->sendrecv.socket != -1
	&& instance->sendrecv.connection_lost == 0)
	{
		sleep(shorter_sleep);
		keep_alive_slept += shorter_sleep;
		attr_cache_slept += shorter_sleep;

		if (instance->client.maintenance_please_die != 0)
		{
			pthread_exit(0);
		}

		if (keep_alive_slept >= client_keep_alive_period()
		&& client_keep_alive_trylock(instance) == 0)
		{
			if (check_connection(instance) == 0)
			{
				rfs_keep_alive(instance);
			}

			client_keep_alive_unlock(instance);
			keep_alive_slept = 0;
		}

		if (attr_cache_slept >= ATTR_CACHE_TTL * 2
		&& client_keep_alive_lock(instance) == 0)
		{
			if (cache_is_old(&instance->attr_cache) != 0)
			{
				clear_cache(&instance->attr_cache);
			}

			client_keep_alive_unlock(instance);
			attr_cache_slept = 0;
		}
	}

	return NULL;
}

void* rfs_init(struct rfs_instance *instance)
{
	client_keep_alive_init(instance);
	if (pthread_create(&instance->client.maintenance_thread, NULL, maintenance, (void *)instance) != 0)
	{
		instance->client.maintenance_thread = 0;
		/* TODO: how to handle ? */
	}

	init_write_behind(instance);

#ifdef WITH_UGO
	if ((instance->client.export_opts & OPT_UGO) != 1)
	{
#if defined RFSNSS_AVAILABLE
	    if (init_nss_server(instance, 0) != 0)
	    {
		    instance->nss.use_nss = 0;
	    }
#endif /* RFSNSS_AVAILABLE */

		create_uids_lookup(&(instance->id_lookup.uids));
		create_gids_lookup(&(instance->id_lookup.gids));
	}
#endif /* WITH_UGO */

#ifdef SCHEDULING_AVAILABLE
	set_scheduler();
#endif

	return NULL;
}
