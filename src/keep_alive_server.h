/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef KEEP_ALIVE_H
#define KEEP_ALIVE_H

/** routines for checking keep alive state on server */

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct rfsd_instance;

/** check if connection is should be closed during lack of activity 
(no operations and no keep alive packets) */
int server_keep_alive_expired(struct rfsd_instance *instance);

/** update last keep alive time with current time() value */
void server_keep_alive_update(struct rfsd_instance *instance);

/** check if process is in the middle of an operation */
int server_keep_alive_locked(struct rfsd_instance *instance);

/** lock keep alive. not needed actually since it is implemented with SIGALRM.
for future use */
int server_keep_alive_lock(struct rfsd_instance *instance);

/** unlock keep alive */
int server_keep_alive_unlock(struct rfsd_instance *instance);

/** get keep alive checking period */
unsigned server_keep_alive_period(void);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* KEEP_ALIVE_H */

