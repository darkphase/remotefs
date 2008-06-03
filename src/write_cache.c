#include "write_cache.h"

#include <string.h>

#include "config.h"
#include "list.h"
#include "buffer.h"

static struct list *cache = NULL;
static const unsigned max_cache_size = 512 * 1024; // bytes
static unsigned cache_size = 0;

const struct list* get_write_cache()
{
	return cache;
}

unsigned get_write_cache_size()
{
	return cache_size;
}

unsigned char is_fit_to_write_cache(unsigned size)
{
	return cache_size + size <= max_cache_size ? 1 : 0;
}

int add_to_write_cache(uint64_t descriptor, const char *buffer, unsigned size)
{
	struct write_cache_entry *entry = get_buffer(sizeof(*entry));

	if (entry == NULL)
	{
		return -1;
	}

	entry->descriptor = descriptor;
	entry->buffer = get_buffer(size);

	if (entry->buffer == NULL)
	{
		free_buffer(entry);

		return -1;
	}

	memcpy(entry->buffer, buffer, size);
	entry->size = size;

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
			free_buffer(entry);
		}

		item = item->next;
	}

	destroy_list(cache);
	cache = NULL;
}

unsigned char is_file_in_write_cache(uint64_t descriptor)
{
	struct list *item = cache;
	while (item != NULL)
	{
		struct write_cache_entry *entry = (struct write_cache_entry *)item->data;

		if (entry != NULL
		&& entry->descriptor == descriptor)
		{
			return 1;
		}

		item = item->next;
	}

	return 0;
}

