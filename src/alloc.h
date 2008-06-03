#ifndef MEMPOOL_H
#define MEMPOOL_H

void *mp_alloc(unsigned size);
void mp_free(void *buffer);
void mp_force_free();

#endif // MEMPOOL_H
