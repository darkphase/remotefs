#ifndef MEMPOOL_H
#define MEMPOOL_H

#include <sys/types.h>

void *mp_alloc(size_t size);
void mp_free(void *buffer);
void mp_force_free();

#endif // MEMPOOL_H
