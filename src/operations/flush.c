/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "../buffer.h"
#include "../command.h"
#include "../config.h"
#include "../data_cache.h"
#include "../instance_client.h"
#include "../list.h"
#include "../sendrecv_client.h"
#include "operations.h"
#include "operations_rfs.h"

int _flush_write(struct rfs_instance *instance, const char *path, uint64_t desc)
{
	DEBUG("flushing file %llu\n", (unsigned long long)desc);
	
	struct write_behind_request *write_behind_request = &instance->write_cache.write_behind_request;

	const struct list *cache_item = instance->write_cache.cache;
	
	while (cache_item != NULL)
	{
		struct cache_block *block = (struct cache_block *)cache_item->data;
		
		if (block->descriptor == desc)
		{
			int ret = 0;
			PARTIALY_DECORATE(ret,
			_write,
			instance, 
			path,
			block->data,
			block->used, 
			block->offset, 
			block->descriptor);
			
			if (ret < 0)
			{
				return ret;
			}

			cache_item = cache_item->next; /* fix list pointer before deletion */

			if (block == write_behind_request->block)
			{
				reset_write_behind(instance);
			}
			else
			{
				delete_block_from_cache(&instance->write_cache.cache, block);
			}

			continue; /* since list pointer is already fixed */
		}
		
		cache_item = cache_item->next;
	}
	
	return 0;
}

int _rfs_flush(struct rfs_instance *instance, const char *path, uint64_t desc)
{
	return _flush_write(instance, path, desc);
}
