/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef WRITE_CACHE_H
#define WRITE_CACHE_H

/** write cache routines */

#include <stdint.h>
#include <sys/types.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/** check if current cache size + given size is less than max cache size */
unsigned is_fit_to_write_cache(uint64_t descriptor, size_t size, off_t offset);

/** copy the buffer and add it to cache */
int add_to_write_cache(uint64_t descriptor, const char *buffer, size_t size, off_t offset);

/** get pointer to data in cache*/
const char* get_write_cache_block();

/** get cached data size */
size_t get_write_cache_size();

/** delete cache */
void destroy_write_cache();

/** check if data stored in cache is for file with specified descriptor */
unsigned write_cache_is_for(uint64_t descriptor);

/** init write cache memory
@param path filename
@param offset first offset in chain of writes
@param size overall cache size
*/
int init_write_cache(const char *path, off_t offset, size_t size);

/** reset inited cache */
void uninit_write_cache();

/** get size of last used cache block */
size_t last_used_write_block();

/** get filename with which cache was inited */
const char *write_cached_path();

/** get max size of write cache */
size_t write_cache_max_size();

/** get offset with which cache was inited */
off_t write_cached_offset();

/** get descriptor for which is data in cache */
uint64_t write_cached_descriptor();

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* WRITE_CACHE_H */
