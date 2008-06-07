#ifndef KEEP_ALIVE_H
#define KEEP_ALIVE_H

int keep_alive_expired();
void update_keep_alive();
int keep_alive_locked();
int keep_alive_lock();
int keep_alive_unlock();
unsigned keep_alive_period();

#endif // KEEP_ALIVE_H
