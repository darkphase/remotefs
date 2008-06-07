#include "write_cache.h"

#include <string.h>

#include "config.h"
#include "list.h"
#include "buffer.h"

static struct list *cache = NULL;
static const unsigned max_cache_size = 512 * 1024; // bytes
static unsigned cache_size = 0;
static uint64_t last_cached_desc = (uint64_t)-1;
static off_t last_cached_offset = (off_t)-1;
static size_t last_cached_size = (size_t)-1;

const struct list* get_write_cache()
{
	return cache;
}

size_t get_write_cache_size()
{
	return cache_size;
}

unsigned is_fit_to_write_cache(uint64_t descriptor, size_t size, off_t offset)
{
	if (cache_size + size > max_cache_size)
	{
		return 0;
	}

	if (last_cached_desc != (uint64_t)-1
	&& descriptor != last_cached_desc)
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

	struct write_cache_entry *entry = get_buffer(sizeof(*entry));

	if (entry == NULL)
	{
		return -1;
	}

	entry->buffer = get_buffer(size);

	if (entry->buffer == NULL)
	{
		free_buffer(entry);

		return -1;
	}

	memcpy(entry->buffer, buffer, size);
	entry->size = size;
	entry->offset = offset;

	struct list *added = add_to_list(cache, entry);
	if (added == NULL)
	{
		free_buffer(entry->buffer);
		free_buffer(entry);
		return -1;
	}

	if (cache == NULL)
	{
		cache = added;
	}
	
	cache_size += size;
	last_cached_desc = descriptor;
	last_cached_size = size;
	last_cached_offset = offset;

	return 0;
}

void destroy_write_cache()
{
	struct list *item = cache;
	while (item != NULL)
	{
		struct write_cache_entry *entry = (struct write_cache_entry *)item->data;

		if (entry != NULL)
		{
			if (entry->buffer != NULL)
			{
				free_buffer(entry->buffer);
			}
		}

		item = item->next;
	}

	destroy_list(cache);
	
	cache = NULL;
	cache_size = 0;
	last_cached_desc = (uint64_t)-1;
	last_cached_size = (size_t)-1;
	last_cached_offset = (off_t)-1;
}
