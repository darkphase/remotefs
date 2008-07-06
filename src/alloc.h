#ifndef MEMPOOL_H
#define MEMPOOL_H

#include <sys/types.h>

/* memory allocator */

/** allocate buffer */
void *mp_alloc(size_t size);

/** free buffer*/
void mp_free(void *buffer);

/** force freeing of (possibly) cached allocated memory*/
void mp_force_free();

#endif // MEMPOOL_H
