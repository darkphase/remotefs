#if ! defined _RFS_MUTEX_
#include <pthread.h>


/************************************************
 * implementation of mutex which can be locked
 * unlocked from different thread.
 * The normal behaviour is that lock/unlock occur
 * within the same thread FreeBSD check this very well.
 * Darwin and Linux are not fully POSIX complant.
 *
 ************************************************/

#define RFS_LOCKED   0
#define RFS_UNLOCKED 1

typedef struct rfs_mutex_s
{
	pthread_cond_t  co;
	pthread_mutex_t mc;
	unsigned int val;
} rfs_mutex_t;

static inline int rfs_mutex_init(rfs_mutex_t *mu,pthread_mutexattr_t *attr)
{
	int ret1;
	int ret2;
	ret1 = pthread_cond_init(&mu->co, NULL);
	ret2 = pthread_mutex_init(&mu->mc, NULL);
	mu->val = RFS_UNLOCKED;
	return ret1 == 0 ? ret1 : ret2;
}

static inline int rfs_mutex_destroy(rfs_mutex_t *mu)
{
	int ret1;
	int ret2;
	ret1 = pthread_mutex_destroy(&mu->mc);
	ret2 = pthread_cond_destroy(&mu->co);
	return ret1 == 0 ? ret1 : ret2;
}

static inline int rfs_mutex_lock(rfs_mutex_t *mu)
{
	int ret;
	int val;
	ret = pthread_mutex_lock(&mu->mc);
	if ( ret == 0 )
	{
		val = mu->val;
		while( mu->val != RFS_UNLOCKED )
		{
			ret = pthread_cond_wait(&mu->co, &mu->mc);
		}
		if ( ret == 0 )
		{
			mu->val = RFS_LOCKED;
		}
		else
		{
			return 1;
		}
		pthread_mutex_unlock(&mu->mc);
		pthread_cond_signal(&mu->co);
		return ret;
	}
	else
	{
		pthread_cond_signal(&mu->co);
		return ret;
	}
}

static inline int rfs_mutex_trylock(rfs_mutex_t *mu)
{
	int ret;
	int val;
	ret = pthread_mutex_lock(&mu->mc);
	if ( ret == 0 )
	{
		val = mu->val;
		if( mu->val == RFS_UNLOCKED )
		{
			mu->val = RFS_LOCKED;
		}
		else
		{
			ret = 1;
		}
		pthread_mutex_unlock(&mu->mc);
		pthread_cond_signal(&mu->co);
		return ret;
	}
	else
	{
		pthread_cond_signal(&mu->co);
		return ret;
	}
}

static inline int rfs_mutex_unlock(rfs_mutex_t *mu)
{
	int ret;
	int val;
	ret = pthread_mutex_lock(&mu->mc);
	if ( ret == 0 )
	{
		val = mu->val;
		while( mu->val != RFS_LOCKED )
		{
			ret = pthread_cond_wait(&mu->co, &mu->mc);
		}
		if ( ret == 0 )
		{
			mu->val = RFS_UNLOCKED;
		}
		pthread_mutex_unlock(&mu->mc);
		pthread_cond_signal(&mu->co);
		return ret;
	}
	else
	{
		pthread_cond_signal(&mu->co);
		return ret;
	}
}

#define pthread_mutex_unlock(a) rfs_mutex_unlock(a)
#define pthread_mutex_lock(a) rfs_mutex_lock(a)
#define pthread_mutex_trylock(a) rfs_mutex_trylock(a)
#define pthread_mutex_init(a,b) rfs_mutex_init(a,b)
#define pthread_mutex_destroy(a) rfs_mutex_destroy(a)
#define pthread_mutex_t rfs_mutex_t

#define _RFS_MUTEX_
#endif
