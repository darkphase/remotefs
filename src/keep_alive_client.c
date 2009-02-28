/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include "config.h"
#include "keep_alive_client.h"
#include "instance_client.h"

unsigned keep_alive_period()
{
	return KEEP_ALIVE_PERIOD;
}

int keep_alive_init(struct rfs_instance *instance)
{
	return pthread_mutex_init(&instance->keep_alive.mutex, NULL);
}

int keep_alive_destroy(struct rfs_instance *instance)
{
	return pthread_mutex_destroy(&instance->keep_alive.mutex);
}

int keep_alive_trylock(struct rfs_instance *instance)
{
	return pthread_mutex_trylock(&instance->keep_alive.mutex);
}

int keep_alive_unlock(struct rfs_instance *instance)
{
	return pthread_mutex_unlock(&instance->keep_alive.mutex);
}

int keep_alive_lock(struct rfs_instance *instance)
{
	return pthread_mutex_lock(&instance->keep_alive.mutex);
}
