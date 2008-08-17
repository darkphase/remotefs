#include "read_cache.h"

#include <string.h>

#include "config.h"
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
	return descriptor == last_cached_desc ? last_cached_size : 0;
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
	last_cached_desc = descriptor;
	last_cached_size = size;
	last_cached_offset = offset;
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
	if (cache != NULL)
	{
		if (last_cached_desc == descriptor)
		{
			return 1;
		}
	}
	
	return 0;
}

char* read_cache_resize(size_t size)
{
	if (last_cached_size != size
	&& cache != NULL)
	{
		DEBUG("%s\n", "need to resize cache");
		free_buffer(cache);
		cache = NULL;	}
	
	if (cache == NULL)
	{
		cache = get_buffer(size);
	}
	
	return cache;
}
