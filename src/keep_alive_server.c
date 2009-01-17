/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <time.h>

#include "config.h"
#include "keep_alive_server.h"
#include "instance.h"

unsigned keep_alive_period()
{
	return KEEP_ALIVE_PERIOD * 2;
}

int keep_alive_expired(struct rfsd_instance *instance)
{
	return ((time(NULL) - instance->keep_alive.last_keep_alive) >= KEEP_ALIVE_PERIOD * 2) ? 0 : -1;
}

void update_keep_alive(struct rfsd_instance *instance)
{
	time(&instance->keep_alive.last_keep_alive);
}

int keep_alive_locked(struct rfsd_instance *instance)
{
	return instance->keep_alive.lock == 1 ? 0 : -1;
}

int keep_alive_lock(struct rfsd_instance *instance)
{
	if (instance->keep_alive.lock == 0)
	{
		instance->keep_alive.lock = 1;
		return 0;
	}
	
	return -1;
}

int keep_alive_unlock(struct rfsd_instance *instance)
{
	instance->keep_alive.lock = 0;
	return 0;
}
