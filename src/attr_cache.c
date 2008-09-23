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

static void *cache = NULL;
static unsigned cache_ttl = ATTR_CACHE_TTL; /* secs */
static time_t last_time_checked = (time_t)0;

static int any_node(const void *s1, const void *s2)
{
	return 0; /* equal */
}

static int compare_path(const void *s1, const void *s2)
{
	return strcmp(((struct tree_item *)s1)->path, ((struct tree_item *)s2)->path);
}

unsigned char cache_is_old()
{
	if (time(NULL) > last_time_checked + cache_ttl)
	{
		last_time_checked = time(NULL);
		return 1;
	}
	
	return 0;
}

static int compare_time(const void *s1, const void *s2)
{
	return ((struct tree_item *)s1)->time > (((struct tree_item *)s2)->time + cache_ttl)
	? 0
	: 1;
}

void clear_cache()
{
	if (cache == NULL)
	{
		return;
	}
	
	DEBUG("%s\n", "clearing cache");
	
	struct tree_item key = { 0 };
	
	key.path = NULL;
	key.time = time(NULL);
	
	do
	{
		void *found = tfind(&key, &cache, compare_time);

		if (found == NULL)
		{
			break;
		}
		
		struct tree_item *node = *(struct tree_item **)found;
		
		if (tdelete(node, &cache, compare_path) != NULL)
		{
			DEBUG("deleted from cache: %s\n", node->path);
			
			free(node->path);
			free_buffer(node);
		}
	}
	while (1);
}

void* cache_file(const char *path, struct stat *stbuf)
{
	struct tree_item *key = get_buffer(sizeof(*key));
	
	key->path = strdup(path);
	key->time = time(NULL);
	memcpy(&(key->data), stbuf, sizeof(key->data));
	
	void *found = tfind(key, &cache, compare_path);
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
		return tsearch(key, &cache, compare_path);
	}
}

struct tree_item* get_cache(const char *path)
{
	struct tree_item key = { (char *)path, time(NULL), { 0 } };
	
	void *found = tfind(&key, &cache, compare_path);
	if (found != NULL)
	{
		return *(struct tree_item **)found;
	}
	
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

void destroy_cache()
{
	if (cache)
	{
		twalk(cache, release_cache);
		
		while (cache != NULL)
		{
			tdelete(NULL, &cache, any_node);
		}
		
		cache = NULL;
	}
}

void delete_from_cache(const char *path)
{
	struct tree_item key = { (char *)path, time(NULL), { 0 } };
	
	tdelete(&key, &cache, compare_path);
}
