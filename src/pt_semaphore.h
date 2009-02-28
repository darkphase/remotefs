/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef PT_SEMAPHORE_H
#define PT_SEMAPHORE_H

#include <pthread.h>

#include "config.h"

/** implementation of semaphore based on pthread */

typedef struct {
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	unsigned int value;
} rfs_sem_t;

static inline int rfs_sem_init(rfs_sem_t *sem, int pshared, unsigned int value)
{
	int ret;
	sem->value = value;
	ret = pthread_mutex_init(&sem->mutex, NULL);
	if ( ret == 0 )
	{
		ret = pthread_cond_init(&sem->cond, NULL);
	}
	return ret;
}

static inline int rfs_sem_post(rfs_sem_t *sem)
{
	int ret;
	ret = pthread_mutex_lock(&sem->mutex);
	if ( ret == 0 )
	{
		sem->value++;
		ret = pthread_cond_signal(&sem->cond);
		pthread_mutex_unlock(&sem->mutex);
	}
	return ret;
}

static inline int rfs_sem_wait(rfs_sem_t *sem)
{
	int ret;
	ret = pthread_mutex_lock(&sem->mutex);
	if ( ret == 0 )
	{
		while( sem->value < 1 )
		{
			ret = pthread_cond_wait(&sem->cond, &sem->mutex);
		}
		if ( ret == 0 )
		{
			sem->value--;
		}
		pthread_mutex_unlock(&sem->mutex);
	}
	return ret;
}

static inline int rfs_sem_destroy(rfs_sem_t *sem)
{
	pthread_cond_destroy(&sem->cond);
	pthread_mutex_destroy(&sem->mutex);
	return 0;
}

#endif /* PT_SEMAPHORE_H */

