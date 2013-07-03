/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef DATA_CACHE_H
#define DATA_CACHE_H

/** file blocks in memory */

#include <sys/types.h>
#include <stdint.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct list;

/** data cache block */
struct cache_block
{
	size_t allocated;
	size_t used;
	uint64_t descriptor;
	off_t offset;
	char *data;
};

/** reserve block of size bytes and add it to list */
struct cache_block* reserve_cache_block(struct list **head, size_t size, off_t offset, uint64_t descriptor);

/** return found block for given file descriptor 
if (allocated - used) > size && block.offset <= offset */
struct cache_block* find_suitable_cache_block(const struct list *head, size_t size, off_t offset, uint64_t descriptor);

/** delete all cache for descriptor */
void clear_cache_by_desc(struct list **head, uint64_t descriptor);

/** delete all cache for (block.offset + used) < offset */
void clear_cache_by_offset(struct list **head, uint64_t descriptor, off_t offset);

/** delete all allocated blocks */
void destroy_data_cache(struct list **head);

/** _calculate_ cache size */
size_t cache_size(const struct list *head);

/** delete given block from cache */
struct list* delete_block_from_cache(struct list **head, struct cache_block *block);

#ifdef RFS_DEBUG
/** print block info */
void dump_block(const struct cache_block *block);
#endif

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* DATA_CACHE_H */
