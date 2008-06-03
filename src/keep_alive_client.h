#ifndef KEEP_ALIVE_CLIENT_H
#define KEEP_ALIVE_CLIENT_H

unsigned keep_alive_period();
int keep_alive_init();
int keep_alive_destroy();
int keep_alive_trylock();
int keep_alive_lock();
int keep_alive_unlock();
int keep_alive_destroy();

#endif // KEEP_ALIVE_CLIENT_H
