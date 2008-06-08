#ifndef READ_CACHE_H
#define READ_CACHE_H

#include <sys/types.h>
#include <stdint.h>

size_t read_cache_have_data(uint64_t descriptor, off_t offset);
const char* read_cache_get_data(uint64_t descriptor, size_t size, off_t offset);
/// will copy the buffer
/// @param descriptor 
/// @param buffer 
/// @param size 
/// @param offset 
/// @return 
int put_to_read_cache(uint64_t descriptor, const char *buffer, size_t size, off_t offset);
void destroy_read_cache();
size_t read_cache_max_size();
size_t read_cache_size(uint64_t descriptor);
void update_read_cache_stats(uint64_t descriptor, size_t size, off_t offset);

#endif // READ_CACHE_H
