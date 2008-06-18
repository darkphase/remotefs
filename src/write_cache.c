#include "write_cache.h"

#include <string.h>
#include <malloc.h>

#include "config.h"
#include "buffer.h"

static char *cache_block = NULL;
static const unsigned max_cache_size = DEFAULT_RW_CACHE_SIZE / 2;
static unsigned cache_size = 0;
static uint64_t last_cached_desc = (uint64_t)-1;
static off_t last_cached_offset = (off_t)-1;
static size_t last_cached_size = (size_t)-1;
static size_t last_used_size = (size_t)-1;
static size_t last_used_offset = (off_t)-1;
static char *last_used_path = NULL;

size_t write_cache_max_size()
{
	return max_cache_size;
}

const char* write_cached_path()
{
	return cache_block != NULL ? last_used_path : NULL;
}

off_t write_cached_offset()
{
	return cache_block != NULL ? last_used_offset : (off_t)-1;
}

uint64_t write_cached_descriptor()
{
	return cache_block != NULL ? last_cached_desc : (uint64_t)-1;
}

int init_write_cache(const char *path, off_t offset, size_t size)
{
	if (cache_block != NULL)
	{
		return -1;
	}
	
	if (size > max_cache_size)
	{
		return -1;
	}
	
	if (size > 0)
	{
		cache_block = get_buffer(size);
	}
	
	last_used_size = size;
	last_used_offset = offset;
	last_used_path = strdup(path);
	
	return 0;
}

void uninit_write_cache()
{
	DEBUG("%s", "uniniting cache\n");
	last_used_size = (size_t)-1;
	last_used_offset = (off_t)-1;
	
	if (last_used_path != NULL)
	{
		free(last_used_path);
		last_used_path = NULL;
	}
}

size_t last_used_write_block()
{
	return last_used_size;
}

const char* get_write_cache_block()
{
	return cache_block;
}

size_t get_write_cache_size()
{
	return cache_block == NULL ? 0 : cache_size;
}

unsigned is_fit_to_write_cache(uint64_t descriptor, size_t size, off_t offset)
{
	if (cache_block == NULL )
	{
		return 0;
	}
	
	if (last_cached_desc != (uint64_t)-1
	&& descriptor != last_cached_desc)
	{
		return 0;
	}
	
	if (cache_size + size > last_used_size
	|| cache_size + size > max_cache_size)
	{
		return 0;
	}
	
	if (last_cached_desc != (uint64_t)-1
	&& last_cached_offset + last_cached_size != offset)
	{
		return 0;
	}
	
	return 1;
}

int add_to_write_cache(uint64_t descriptor, const char *buffer, size_t size, off_t offset)
{
	if (last_cached_desc != (uint64_t)-1
	&& descriptor != last_cached_desc)
	{
		return -1;
	}
	
	if (is_fit_to_write_cache(descriptor, size, offset) == 0)
	{
		return -1;
	}

	memcpy(cache_block + (offset - last_used_offset), buffer, size);

	cache_size += size;
	last_cached_desc = descriptor;
	last_cached_size = size;
	last_cached_offset = offset;

	return 0;
}

void destroy_write_cache()
{
	cache_size = 0;
	last_cached_desc = (uint64_t)-1;
	last_cached_size = (size_t)-1;
	last_cached_offset = (off_t)-1;
	
	if (cache_block != NULL)
	{
		free_buffer(cache_block);
		cache_block = NULL;
	}
	
	if (last_used_path != NULL)
	{
		free(last_used_path);
		last_used_path = NULL;
	}
}

unsigned write_cache_is_for(uint64_t descriptor)
{
	if (cache_block != 0)
	{
		if (last_cached_desc == descriptor)
		{
			return 1;
		}
	}
	
	return 0;
}
