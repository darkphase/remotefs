#ifndef CACHE_H
#define CACHE_H

#include <sys/stat.h>
#include <time.h>

/* attributes cache. used with rfs_getattr()
*
*/

/** tree_item is data type stored in cache 
which is balanced tree with filename as key */
struct tree_item
{
	char *path;
	time_t time;
	struct stat data;
};

/** add file to cache */
void* cache_file(const char *path, struct stat *stbuf);

/** delete file from cache */
void delete_from_cache(const char *path);

/** get cached value for file */
struct tree_item* get_cache(const char *path);

/** delete all cached data */
void destroy_cache();

/** check if cache is outdated
@return not 0 if cache is old 
*/
unsigned char cache_is_old();

/** delete outdated files from cache */
void clear_cache();

#endif // CACHE_H
