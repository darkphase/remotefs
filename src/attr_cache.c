/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <search.h>
#include <stdlib.h>
#include <string.h>

#include "attr_cache.h"
#include "buffer.h"
#include "config.h"
#include "instance_client.h"
#ifdef RFS_DEBUG
#	include "ratio.h"
#endif

static int any_node(const void *s1, const void *s2)
{
	return 0; /* equal */
}

static int compare_path(const void *s1, const void *s2)
{
	return strcmp(((struct tree_item *)s1)->path, ((struct tree_item *)s2)->path);
}

static int compare_time(const void *s1, const void *s2)
{
	return ((struct tree_item *)s1)->time > (((struct tree_item *)s2)->time + ATTR_CACHE_TTL)
	? 0
	: 1;
}

unsigned cache_is_old(struct attr_cache *cache)
{
	time_t now = time(NULL);
	if (now > cache->last_time_checked + ATTR_CACHE_TTL)
	{
		cache->last_time_checked = now;
		return 1;
	}
	
	return 0;
}

void clear_cache(struct attr_cache *cache)
{
	if (cache->cache == NULL)
	{
		return;
	}
	
	DEBUG("%s\n", "clearing cache");
	
	struct tree_item key = { 0 };
	
	key.path = NULL;
	key.time = time(NULL);
	
	do
	{
		void *found = tfind(&key, &cache->cache, compare_time);
		
		if (found == NULL)
		{
			break;
		}
		
		struct tree_item *node = *(struct tree_item **)found;
		
		if (tdelete(node, &cache->cache, compare_path) != NULL)
		{
			DEBUG("deleted from cache: %s\n", node->path);
			
			free(node->path);
			free(node);
		}
	}
	while (1);
}

void* cache_file(struct attr_cache *cache, const char *path, struct stat *stbuf)
{
	if (cache->number_of_entries >= ATTR_CACHE_MAX_ENTRIES)
	{
		return NULL;
	}

	struct tree_item *key = malloc(sizeof(*key));

	if (key == NULL)
	{
		return NULL;
	}
	
	key->path = strdup(path);
	key->time = time(NULL);
	memcpy(&(key->data), stbuf, sizeof(key->data));
	
	void *found = tfind(key, &cache->cache, compare_path);
	if (found != NULL)
	{
		free(key->path);
		free(key);
		
		struct tree_item *value = *(struct tree_item **)found;
		memcpy(&value->data, stbuf, sizeof(value->data));
		value->time = time(NULL);
		
		return found;
	}
	else
	{
		++(cache->number_of_entries);
#ifdef RFS_DEBUG
		if (cache->number_of_entries > cache->max_number_of_entries)
		{
			cache->max_number_of_entries = cache->number_of_entries;
		}
#endif
		return tsearch(key, &cache->cache, compare_path);
	}
}

const struct tree_item* 
get_cache(struct attr_cache *cache, const char *path)
{
	struct tree_item key = { (char *)path, time(NULL), { 0 } };
	
	void *found = tfind(&key, &cache->cache, compare_path);
	if (found != NULL)
	{
#ifdef RFS_DEBUG
		++cache->hits;
#endif
		return *(struct tree_item **)found;
	}
#ifdef RFS_DEBUG
	else
	{
		++cache->misses;
	}
#endif
	
	return NULL;
}

static void release_cache(const void *nodep, const VISIT which, const int depth)
{
	if (which == endorder 
	|| which == leaf)
	{
		struct tree_item *node = *(struct tree_item **)nodep;
		
		if (node)
		{
			if (node->path)
			{
				free(node->path);
			}
			
			free(node);
		}
	}
}

void* destroy_cache(struct attr_cache *cache)
{
	if (cache->cache != NULL)
	{
		twalk(cache->cache, release_cache);
		
		while (cache->cache != NULL)
		{
			tdelete(NULL, &cache->cache, any_node);
		}
	}

	return NULL;
}

void delete_from_cache(struct attr_cache *cache, const char *path)
{
	struct tree_item key = { (char *)path, time(NULL), { 0 } };

	void *found = tfind(&key, &cache->cache, compare_path);
	struct tree_item *value = (found != NULL ? *(struct tree_item **)found : NULL);
	
	tdelete(&key, &cache->cache, compare_path);
	
	if (value != NULL)
	{
		free(value->path);
		free(value);
		--(cache->number_of_entries);
	}
}

#ifdef RFS_DEBUG
void dump_attr_stats(struct attr_cache *cache)
{
	DEBUG("attr cache hits: %lu, misses: %lu, ratio: %.2f\n", 
	cache->hits, 
	cache->misses, 
	ratio(cache->hits, cache->misses));
	DEBUG("attr cache max number of entries: %lu/%u\n", cache->max_number_of_entries, ATTR_CACHE_MAX_ENTRIES);
}
#endif
