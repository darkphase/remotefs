#ifndef KEEP_ALIVE_CLIENT_H
#define KEEP_ALIVE_CLIENT_H

/* routines for sending keep alive from client */

/** get period of keep alive packets sending */
unsigned keep_alive_period();

/** init keep alive data */
int keep_alive_init();

/** destroy any previously allocated data for keep alive
@see keep_alive_init
*/
int keep_alive_destroy();

/** try lock keep alive mutex */
int keep_alive_trylock();

/** lock keep alive mutex. will block process until lock is successful */
int keep_alive_lock();

/** unlock keep alive mutex */
int keep_alive_unlock();

#endif // KEEP_ALIVE_CLIENT_H
