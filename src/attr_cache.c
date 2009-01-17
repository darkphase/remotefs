/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <search.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "config.h"
#include "attr_cache.h"
#include "buffer.h"
#include "instance.h"

#ifdef RFS_DEBUG
#include "ratio.h"
#endif

static int any_node(const void *s1, const void *s2)
{
	return 0; /* equal */
}

static int compare_path(const void *s1, const void *s2)
{
	return strcmp(((struct tree_item *)s1)->path, ((struct tree_item *)s2)->path);
}

unsigned char cache_is_old(struct rfs_instance *instance)
{
	if (time(NULL) > instance->attr_cache.last_time_checked + ATTR_CACHE_TTL)
	{
		instance->attr_cache.last_time_checked = time(NULL);
		return 1;
	}
	
	return 0;
}

static int compare_time(const void *s1, const void *s2)
{
	return ((struct tree_item *)s1)->time > (((struct tree_item *)s2)->time + ATTR_CACHE_TTL)
	? 0
	: 1;
}

void clear_cache(struct rfs_instance *instance)
{
	if (instance->attr_cache.cache == NULL)
	{
		return;
	}
	
	DEBUG("%s\n", "clearing cache");
	
	struct tree_item key = { 0 };
	
	key.path = NULL;
	key.time = time(NULL);
	
	do
	{
		void *found = tfind(&key, &instance->attr_cache.cache, compare_time);
		
		if (found == NULL)
		{
			break;
		}
		
		struct tree_item *node = *(struct tree_item **)found;
		
		if (tdelete(node, &instance->attr_cache.cache, compare_path) != NULL)
		{
			DEBUG("deleted from cache: %s\n", node->path);
			
			free(node->path);
			free_buffer(node);
		}
	}
	while (1);
}

void* cache_file(struct rfs_instance *instance, const char *path, struct stat *stbuf)
{
	struct tree_item *key = get_buffer(sizeof(*key));
	
	key->path = strdup(path);
	key->time = time(NULL);
	memcpy(&(key->data), stbuf, sizeof(key->data));
	
	void *found = tfind(key, &instance->attr_cache.cache, compare_path);
	if (found != NULL)
	{
		free(key->path);
		free_buffer(key);
		
		struct tree_item *value = *(struct tree_item **)found;
		memcpy(&value->data, stbuf, sizeof(value->data));
		value->time = time(NULL);
		
		return found;
	}
	else
	{
		return tsearch(key, &instance->attr_cache.cache, compare_path);
	}
}

const struct tree_item* get_cache(struct rfs_instance *instance, const char *path)
{
	struct tree_item key = { (char *)path, time(NULL), { 0 } };
	
	void *found = tfind(&key, &instance->attr_cache.cache, compare_path);
	if (found != NULL)
	{
#ifdef RFS_DEBUG
		++instance->attr_cache.cache_hits;
#endif
		return *(struct tree_item **)found;
	}
#ifdef RFS_DEBUG
	else
	{
		++instance->attr_cache.cache_misses;
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
			
			free_buffer(node);
		}
	}
}

void destroy_cache(struct rfs_instance *instance)
{
	if (instance->attr_cache.cache != NULL)
	{
		twalk(instance->attr_cache.cache, release_cache);
		
		while (instance->attr_cache.cache != NULL)
		{
			tdelete(NULL, &instance->attr_cache.cache, any_node);
		}
		
		instance->attr_cache.cache = NULL;
	}
}

void delete_from_cache(struct rfs_instance *instance, const char *path)
{
	struct tree_item key = { (char *)path, time(NULL), { 0 } };
	void *found = tfind(&key, &instance->attr_cache.cache, compare_path);
	struct tree_item *value = (found != NULL ? *(struct tree_item **)found : NULL);
	
	tdelete(&key, &instance->attr_cache.cache, compare_path);
	
	if (value != NULL)
	{
		free(value->path);
		free_buffer(value);
	}
}

#ifdef RFS_DEBUG
void dump_attr_stats(struct rfs_instance *instance)
{
	DEBUG("attr cache hits: %lu, misses: %lu, ratio: %.2f\n", 
	instance->attr_cache.cache_hits, 
	instance->attr_cache.cache_misses, 
	ratio(instance->attr_cache.cache_hits, instance->attr_cache.cache_misses));
}
#endif
