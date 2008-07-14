#ifndef MEMPOOL_H
#define MEMPOOL_H

/* memory allocator */

#include <sys/types.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/** allocate buffer */
void *mp_alloc(size_t size);

/** free buffer*/
void mp_free(void *buffer);

/** force freeing of (possibly) cached allocated memory*/
void mp_force_free();

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif // MEMPOOL_H
