/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <string.h>

#include "config.h"
#include "read_cache.h"
#include "buffer.h"

static size_t max_cache_size = DEFAULT_RW_CACHE_SIZE;
static char *cache = NULL;
static off_t last_cached_offset = (off_t)-1;
static size_t last_cached_size = (size_t)-1;
static uint64_t last_cached_desc = (uint64_t)-1;

void read_cache_force_max(size_t size)
{
	DEBUG("read cache max size is set to %d\n", size);
	max_cache_size = size;
}

size_t read_cache_max_size()
{
	return max_cache_size;
}

size_t read_cache_size(uint64_t descriptor)
{
	return ((cache != NULL && descriptor == last_cached_desc) ? last_cached_size : 0);
}

size_t last_used_read_block(uint64_t descriptor)
{
	return (descriptor == last_cached_desc) ? last_cached_size : 0;
}

size_t read_cache_have_data(uint64_t descriptor, off_t offset)
{
	if (cache == NULL 
	|| descriptor != last_cached_desc 
	|| offset >= last_cached_offset + last_cached_size
	|| offset < last_cached_offset)
	{
		return 0;
	}
	
	return (last_cached_offset + last_cached_size) - offset;
}

const char* read_cache_get_data(uint64_t descriptor, size_t size, off_t offset)
{
	if (read_cache_have_data(descriptor, offset) >= size)
	{
		return cache + (offset - last_cached_offset);
	}
	
	return NULL;
}

void update_read_cache_stats(uint64_t descriptor, size_t size, off_t offset)
{
	last_cached_desc = size > 0 ? descriptor : -1;
	last_cached_size = size > 0 ? size : -1;
	last_cached_offset = size > 0 ? offset : -1;
	DEBUG("updated read cache stats: %llu, %u, %llu\n", last_cached_desc, last_cached_size, last_cached_offset);
}

void destroy_read_cache()
{
	if (cache != NULL)
	{
		free_buffer(cache);
		
		cache = NULL;
	}
}

unsigned read_cache_is_for(uint64_t descriptor)
{
	if (last_cached_desc == descriptor)
	{
		return 1;
	}
	
	return 0;
}

char* read_cache_resize(size_t size)
{
	if (size == 0)
	{
		return cache;
	}

	if (last_cached_size != size)
	{
		DEBUG("need to resize cache to %u\n", size);
		if (cache != NULL)
		{
			free_buffer(cache);
			cache = NULL;
		}
		
		/* break cache stats after resize
		but don't touch descriptor and size because it may be used
		to determine next cache size for cached file */
		last_cached_offset = (off_t)-1;
	}
	
	if (cache == NULL)
	{
		cache = get_buffer(size);
	}
	
	return cache;
}
