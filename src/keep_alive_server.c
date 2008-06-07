#include "keep_alive_server.h"

#include <time.h>

#include "config.h"

static unsigned int lock = 0;
static time_t last_keep_alive = 0;

unsigned keep_alive_period()
{
	return KEEP_ALIVE_PERIOD * 2;
}

int keep_alive_expired()
{
	return ((time(NULL) - last_keep_alive) >= KEEP_ALIVE_PERIOD * 2) ? 0 : -1;
}

void update_keep_alive()
{
	time(&last_keep_alive);
}

int keep_alive_trylock()
{
	if (lock == 0)
	{
		lock = 1;
		return 0;
	}
	return -1;
}

int keep_alive_lock()
{
	if (lock == 0)
	{
		lock = 1;
		return 0;
	}
	
	return -1;
}

int keep_alive_unlock()
{
	lock = 0;
	return 0;
}
