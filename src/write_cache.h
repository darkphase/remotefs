#ifndef WRITE_CACHE_H
#define WRITE_CACHE_H

#include <stdint.h>
#include <sys/types.h>

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

const char* get_write_cache_block();
/// get cached data size
/// @return 
size_t get_write_cache_size();
/// delete all chunks stored in cache
/// @return 
void destroy_write_cache();

unsigned write_cache_is_for(uint64_t descriptor);

int init_write_cache(const char *path, off_t offset, size_t size);
void uninit_write_cache();
size_t last_used_write_block();
const char *write_cached_path();
size_t write_cache_max_size();
off_t write_cached_offset();
uint64_t write_cached_descriptor();

#endif // WRITE_CACHE_H
