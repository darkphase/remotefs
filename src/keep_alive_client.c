#include "keep_alive_client.h"

#include <pthread.h>

#include "config.h"

static pthread_mutex_t keep_alive_mutex;

unsigned keep_alive_period()
{
	return KEEP_ALIVE_PERIOD;
}

int keep_alive_init()
{
	return pthread_mutex_init(&keep_alive_mutex, NULL);
}

int keep_alive_destroy()
{
	return pthread_mutex_destroy(&keep_alive_mutex);
}

int keep_alive_trylock()
{
	return pthread_mutex_trylock(&keep_alive_mutex);
}

int keep_alive_unlock()
{
	return pthread_mutex_unlock(&keep_alive_mutex);
}

int keep_alive_lock()
{
	return pthread_mutex_lock(&keep_alive_mutex);
}
