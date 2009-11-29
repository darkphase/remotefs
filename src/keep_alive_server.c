/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include "config.h"
#include "instance_server.h"

unsigned server_keep_alive_period()
{
	return KEEP_ALIVE_PERIOD * 2;
}

int server_keep_alive_expired(struct rfsd_instance *instance)
{
	return ((time(NULL) - instance->keep_alive.last_keep_alive) >= KEEP_ALIVE_PERIOD * 2) ? 0 : -1;
}

void server_keep_alive_update(struct rfsd_instance *instance)
{
	time(&instance->keep_alive.last_keep_alive);
}

int server_keep_alive_locked(struct rfsd_instance *instance)
{
	return instance->keep_alive.lock == 1 ? 0 : -1;
}

int server_keep_alive_lock(struct rfsd_instance *instance)
{
	if (instance->keep_alive.lock == 0)
	{
		instance->keep_alive.lock = 1;
		return 0;
	}
	
	return -1;
}

int server_keep_alive_unlock(struct rfsd_instance *instance)
{
	instance->keep_alive.lock = 0;
	return 0;
}

