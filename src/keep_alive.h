#ifndef KEEP_ALIVE_H
#define KEEP_ALIVE_H

#include <time.h>

unsigned int keep_alive_expired();
void update_keep_alive();
unsigned int keep_alive_locked();
int get_keep_alive_lock();
void release_keep_alive_lock();
unsigned keep_alive_period();

#endif // KEEP_ALIVE_H
