#ifndef READ_CACHE_H
#define READ_CACHE_H

/** read cache routines */

#include <sys/types.h>
#include <stdint.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

size_t read_cache_have_data(uint64_t descriptor, off_t offset);
const char* read_cache_get_data(uint64_t descriptor, size_t size, off_t offset);
void destroy_read_cache();
size_t read_cache_max_size();
size_t read_cache_size(uint64_t descriptor);
void update_read_cache_stats(uint64_t descriptor, size_t size, off_t offset);
size_t last_used_read_block(uint64_t descriptor);
unsigned read_cache_is_for(uint64_t descriptor);
char* read_cache_resize();
void read_cache_force_max(size_t size);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* READ_CACHE_H */
