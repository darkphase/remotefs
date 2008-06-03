#ifndef CACHE_H
#define CACHE_H

#include <sys/stat.h>
#include <time.h>

struct tree_item
{
	char *path;
	time_t time;
	struct stat data;
};

void* cache_file(const char *path, struct stat *stbuf);
void delete_from_cache(const char *path);
struct tree_item* get_cache(const char *path);
void destroy_cache();
unsigned char cache_is_old();
void clear_cache();

#endif // CACHE_H
