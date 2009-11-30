/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef CACHE_H
#define CACHE_H

/** attributes cache. used with rfs_getattr() and rfs_readdir() */

#include <sys/stat.h>
#include <time.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/** attributes cache info */
struct attr_cache
{
	void *cache;
	time_t last_time_checked;
	unsigned long number_of_entries;
#ifdef RFS_DEBUG
	unsigned long hits;
	unsigned long misses;
	unsigned long max_number_of_entries;
#endif
};

/** tree_item is data type stored in cache 
which is balanced tree with filename as key */
struct tree_item
{
	char *path;
	time_t time;
	struct stat data;
};

/** add file to cache */
void* cache_file(struct attr_cache *cache, const char *path, struct stat *stbuf);

/** delete file from cache */
void delete_from_cache(struct attr_cache *cache, const char *path);

/** get cached value for file */
const struct tree_item* get_cache(struct attr_cache *cache, const char *path);

/** delete all cached data */
void* destroy_cache(struct attr_cache *cache);

/** check if cache is outdated
@return not 0 if cache is old 
\param last_time_checked last check time - will be updated with time() 
*/
unsigned cache_is_old(struct attr_cache *cache);

/** delete outdated files from cache */
void clear_cache(struct attr_cache *cache);

#ifdef RFS_DEBUG
/** print hits/misses */
void dump_attr_stats(struct attr_cache *cache);
#endif

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* CACHE_H */

