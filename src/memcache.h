/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifdef WITH_MEMCACHE

#ifndef MEMCACHE_H
#define MEMCACHE_H

/** memory cached read-ahead on server */

#include <stdint.h>
#include <sys/types.h>

#include "buffer.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct memcached_block
{
	uint64_t desc;
	off_t offset;
	size_t size;
	char *data;
};

static inline 
unsigned memcache_fits(const struct memcached_block *cache, uint64_t desc, off_t offset, size_t size)
{
	return (cache != NULL 
	&& cache->desc == desc 
	&& cache->offset == offset 
	&& cache->size == size); /* very basic check, but it should be fine for sequential file reading optimization */
}

static inline 
int memcache_block(struct memcached_block *cache, uint64_t desc, off_t offset, size_t size, char *data)
{
	if (data != NULL 
	&& cache->data != NULL)
	{
		free_buffer(cache->data);
	}

	cache->desc = desc;
	cache->offset = offset;
	cache->size = size;

	if (data != NULL)
	{
		cache->data = data;
	}

	return 0;
}

static inline 
void destroy_memcache(struct memcached_block *cache)
{
	if (cache->data != NULL)
	{
		free_buffer(cache->data);
		cache->data = NULL;
	}

	cache->desc = (uint64_t)(-1);
	cache->offset = (off_t)(-1);
	cache->size = 0;
}

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* MEMCACHE_H */
#endif /* WITH_MEMCACHE */

