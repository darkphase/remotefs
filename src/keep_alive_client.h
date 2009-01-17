/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef KEEP_ALIVE_CLIENT_H
#define KEEP_ALIVE_CLIENT_H

/** routines for sending keep alive from client */

#include <pthread.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct rfs_instance;

/** get period of keep alive packets sending */
unsigned keep_alive_period(void);

/** init keep alive data */
int keep_alive_init(struct rfs_instance *instance);

/** destroy any previously allocated data for keep alive
@see keep_alive_init
*/
int keep_alive_destroy(struct rfs_instance *instance);

/** try lock keep alive mutex */
int keep_alive_trylock(struct rfs_instance *instance);

/** lock keep alive mutex. will block process until lock is successful */
int keep_alive_lock(struct rfs_instance *instance);

/** unlock keep alive mutex */
int keep_alive_unlock(struct rfs_instance *instance);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* KEEP_ALIVE_CLIENT_H */
