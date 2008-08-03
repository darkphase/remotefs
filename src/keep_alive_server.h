#ifndef KEEP_ALIVE_H
#define KEEP_ALIVE_H

/** routines for checking keep alive state on server */

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/** check if connection is should be closed during lack of activity 
(no operations and no keep alive packets) 
*/
int keep_alive_expired();

/** update last keep alive time with current time() value */
void update_keep_alive();

/** check if process is in the middle of an operation */
int keep_alive_locked();

/** lock keep alive. not needed actually since it is implemented with SIGALRM.
for future use
*/
int keep_alive_lock();

/** unlock keep alive */
int keep_alive_unlock();

/** get keep alive checking period */
unsigned keep_alive_period();

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* KEEP_ALIVE_H */
