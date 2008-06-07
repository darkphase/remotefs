#ifndef WRITE_CACHE_H
#define WRITE_CACHE_H

#include <stdint.h>
#include <sys/types.h>

struct list;

struct write_cache_entry
{
	char *buffer;
	off_t offset;
	size_t size;
};

/// check if current cache size + given size is less than max cache size
/// @param size 
/// @return 
unsigned is_fit_to_write_cache(uint64_t descriptor, size_t size, off_t offset);
/// copy the buffer and add it to cache
/// @param descriptor 
/// @param buffer 
/// @param size 
/// @return 
int add_to_write_cache(uint64_t descriptor, const char *buffer, size_t size, off_t offset);
/// get cache as list of chunks
/// @return 
const struct list* get_write_cache();
/// get cached data size
/// @return 
size_t get_write_cache_size();
/// delete all chunks stored in cache
/// @return 
void destroy_write_cache();

#endif // WRITE_CACHE_H
