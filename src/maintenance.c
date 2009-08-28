/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <pthread.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "cookies.h"
#include "maintenance.h"
#include "server.h"

static void* maintenance_proc(void *config_casted)
{
	struct config *config = (struct config *)config_casted;

	while (config->stop_maintenance_thread == 0)
	{
		sleep(1);

		maintenance_lock(config);

		clear_cookies(&config->user_cookies);
		clear_cookies(&config->group_cookies);

		/* check if connections counter is 0 for too long */
		DEBUG("connections counter: %u\n", config->connections);
		if (config->connections == 0 
		&& time(NULL) > config->connections_updated + ZERO_CONNECTIONS_TTL)
		{
			DEBUG("%s\n", "no connections for too long");

			do_stop_server(config, 0);

			maintenance_unlock(config);
			break;
		}

		maintenance_unlock(config);
	}

	return NULL;
}

int maintenance_lock(struct config *config)
{
	DEBUG("%s\n", "locking maintenance mutex");
	return (pthread_mutex_lock(&config->maintenance_lock) == 0 ? 0 : -1);
}

int maintenance_unlock(struct config *config)
{
	DEBUG("%s\n", "unlocking maintenance mutex");
	return (pthread_mutex_unlock(&config->maintenance_lock) == 0 ? 0 : -1);
}

int start_maintenance_thread(struct config *config)
{
	DEBUG("%s\n", "starting maintenance thread");

	if (pthread_create(&config->maintenance_thread, NULL, maintenance_proc, (void *)config) != 0)
	{
		return -1;
	}
	
	return 0;
}

void stop_maintenance_thread(struct config *config)
{
	DEBUG("stopping maintenance thread %ld\n", config->maintenance_thread);

	if (config->maintenance_thread != 0)
	{
		maintenance_lock(config);
		config->stop_maintenance_thread = 1;
		maintenance_unlock(config);

		pthread_join(config->maintenance_thread, NULL);
		config->maintenance_thread = 0;
	}
}

