/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <string.h>

#include "config.h"
#include "data_cache.h"
#include "buffer.h"
#include "list.h"

struct cache_block* reserve_cache_block(struct list **head, size_t size, off_t offset, uint64_t descriptor)
{
	struct cache_block *reserved = get_buffer(sizeof(*reserved));
	
	reserved->allocated = size;
	reserved->used = 0;
	reserved->descriptor = descriptor;
	reserved->offset = offset;
	reserved->data = NULL;
	
	reserved->data = get_buffer(size);
	if (reserved->data == NULL)
	{
		free_buffer(reserved);
		return NULL;
	}
	
	/* memset(reserved->data, 0, size); */
	
	if (add_to_list(head, reserved) == NULL)
	{
		free_buffer(reserved->data);
		free_buffer(reserved);
		return NULL;
	}
	
	return reserved;
}

struct cache_block* find_suitable_cache_block(const struct list *head, size_t size, off_t offset, uint64_t descriptor)
{
	DEBUG("searching block with size %u, offset %llu for desc %llu\n", (unsigned int)size, (unsigned long long)offset, (unsigned long long)descriptor);
	
	const struct list *item = head;
	while (item != NULL)
	{
		struct cache_block *block = (struct cache_block *)item->data;
		
		DEBUG("current block of size %u (%u), offset %llu for desc %llu at %p\n", 
		(unsigned int)block->allocated,
		(unsigned int)block->used,
		(unsigned long long)block->offset,
		(unsigned long long)block->descriptor,
		block);
		
		if (block->descriptor == descriptor
		&& block->offset <= offset
		&& block->offset + block->allocated >= offset + size)
		{
			return block;
		}
		
		item = item->next;
	}
	
	return NULL;
}

void clear_cache_by_desc(struct list **head, uint64_t descriptor)
{
	struct list *item = *head;
	while (item != NULL)
	{
		struct cache_block *block = (struct cache_block *)item->data;
		if (block->descriptor == descriptor)
		{
			free_buffer(block->data);
			item = remove_from_list(head, item);
		}
		else
		{
			item = item->next;
		}
	}
}

void clear_cache_by_offset(struct list **head, uint64_t descriptor, off_t offset)
{
	struct list *item = *head;
	while (item != NULL)
	{
		struct cache_block *block = (struct cache_block *)item->data;
		if (block->descriptor == descriptor
		&& block->offset <= offset)
		{
			free_buffer(block->data);
			item = remove_from_list(head, item);
		}
		else
		{
			item = item->next;
		}
	}
}

void destroy_data_cache(struct list **head)
{
	struct list *item = *head;
	while (item != NULL)
	{
		struct cache_block *block = (struct cache_block *)item->data;
		free_buffer(block->data);
		
		item = item->next;
	}
	
	destroy_list(head);
	*head = NULL;
}

size_t cache_size(const struct list *head)
{
	size_t size = 0;
	const struct list *item = head;
	while (item != NULL)
	{
		size += ((struct cache_block *)item->data)->allocated;
		item = item->next;
	}
	
	return size;
}

struct list* delete_block_from_cache(struct list **head, struct cache_block *block)
{
	struct list *item = *head;
	while (item != NULL)
	{
		if ((struct cache_block *)item->data == block)
		{
			free_buffer(block->data);
			return remove_from_list(head, item);
		}
		
		item = item->next;
	}
	
	return *head;
}

#ifdef RFS_DEBUG
void dump_block(const struct cache_block *block)
{
	DEBUG("dumping cache block at %p\n", block);
	if (block != NULL)
	{
		DEBUG("descriptor: %llu, allocated: %u, used: %u, offset: %llu, data: %p\n",
		(unsigned long long)block->descriptor,
		(unsigned int)block->allocated,
		(unsigned int)block->used,
		(unsigned long long)block->offset,
		block->data);
	}
}
#endif
