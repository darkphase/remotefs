/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "attr_cache.h"
#include "buffer.h"
#include "command.h"
#include "config.h"
#include "data_cache.h"
#include "instance_client.h"
#include "keep_alive_client.h"
#include "list.h"
#include "operations.h"
#include "operations_rfs.h"
#include "psemaphore.h"
#include "sendrecv.h"

static int _write(struct rfs_instance *instance, const char *path, const char *buf, size_t size, off_t offset, uint64_t desc);
static void* write_behind(void *void_instance);

static void reset_write_behind(struct rfs_instance *instance)
{
	DEBUG("%s\n", "reseting write behind");
	
	if (instance->write_cache.write_behind_request.block != NULL)
	{
		delete_block_from_cache(&instance->write_cache.cache, instance->write_cache.write_behind_request.block);
	}
	
	instance->write_cache.write_behind_request.block = NULL;
	instance->write_cache.write_behind_request.please_die = 0;
	instance->write_cache.write_behind_request.last_ret = 0;
	instance->write_cache.write_behind_request.path = NULL;
}

int init_write_behind(struct rfs_instance *instance)
{
	DEBUG("%s\n", "initing write behind");
	
	reset_write_behind(instance);

	DEBUG("%s\n", "initing semaphores");
	
	errno = 0;
	if (rfs_sem_init(&instance->write_cache.write_behind_sem, 0, 0) != 0)
	{
		return -errno;
	}

	DEBUG("%s\n", "creating write behind thread");
	
	errno = 0;
	if (pthread_create(&instance->write_cache.write_behind_thread, NULL, write_behind, (void *)instance) != 0)
	{
		return -errno;
	}
	
	return 0;
}

void kill_write_behind(struct rfs_instance *instance)
{
	DEBUG("%s\n", "killing write behind");
	
	instance->write_cache.write_behind_request.please_die = 1;
	
	DEBUG("%s\n", "unlocking mutex");
	if (rfs_sem_post(&instance->write_cache.write_behind_sem) != 0)
	{
		return;
	}
	
	DEBUG("%s\n", "waiting for thread to end");
	pthread_join(instance->write_cache.write_behind_thread, NULL);
	rfs_sem_destroy(&instance->write_cache.write_behind_sem);

	if (instance->write_cache.write_behind_request.path != NULL)
	{
		free(instance->write_cache.write_behind_request.path);
	}
}

static void* write_behind(void *void_instance)
{
	struct rfs_instance *instance = (struct rfs_instance *)(void_instance);
	struct write_behind_request *write_behind_request = &instance->write_cache.write_behind_request;
	
	if (rfs_sem_init(&instance->write_cache.write_behind_started, 0, 0) != 0)
	{
		return NULL;
	}
	
	DEBUG("%s\n", "write behind started. waiting for request");
	
	while (1)
	{
	
	if (rfs_sem_wait(&instance->write_cache.write_behind_sem) != 0)
	{
		pthread_exit(NULL);
	}
	
	if (write_behind_request->please_die != 0)
	{
		DEBUG("%s\n", "exiting write behind");
		pthread_exit(NULL);
	}
	
	if (keep_alive_lock(instance) != 0)
	{
		write_behind_request->last_ret = -EIO;
		pthread_exit(NULL);
	}
	
	DEBUG("%s\n", "*** write behind started");
	
	rfs_sem_post(&instance->write_cache.write_behind_started);
	
	if (write_behind_request->block == NULL) /* oops. was it already flush'ed? */
	{
		DEBUG("%s\n", "*** write behind finished");
		keep_alive_unlock(instance);
		continue;
	}

	write_behind_request->last_ret = 0;
	
	DEBUG("%s\n", "received write behind request:");
	DEBUG("size: %u, offset: %llu, descriptor: %llu, block at %p\n", 
	(unsigned int )write_behind_request->block->used,
	(unsigned long long)write_behind_request->block->offset,
	(unsigned long long)write_behind_request->block->descriptor,
	write_behind_request->block);
	
	write_behind_request->last_ret = 0;
	PARTIALY_DECORATE(write_behind_request->last_ret,
	_write,
	instance, 
	write_behind_request->path,
	write_behind_request->block->data, 
	write_behind_request->block->used,
	write_behind_request->block->offset,
	write_behind_request->block->descriptor);
	
	delete_block_from_cache(&instance->write_cache.cache, write_behind_request->block);
	free(write_behind_request->path);
	write_behind_request->path = NULL;
	
	DEBUG("%s\n", "*** write behind finished");
	
	keep_alive_unlock(instance);
	
	} /* while (1) */
}

int flush_write(struct rfs_instance *instance, const char *path, uint64_t descriptor)
{
	DEBUG("flushing file %llu\n", (unsigned long long)descriptor);
	
	struct write_behind_request *write_behind_request = &instance->write_cache.write_behind_request;
	
	const struct list *cache_item = instance->write_cache.cache;
	
	while (cache_item != NULL)
	{
		struct cache_block *block = (struct cache_block *)cache_item->data;
		
		if (block->descriptor == descriptor)
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
			
			delete_block_from_cache(&instance->write_cache.cache, block);
		}
		
		cache_item = cache_item->next;
	}
	
	if (write_behind_request->block != NULL
	&& write_behind_request->block->descriptor == descriptor)
	{
		reset_write_behind(instance);
	}
	
	return 0;
}

int _rfs_flush(struct rfs_instance *instance, const char *path, uint64_t desc)
{
	if (keep_alive_lock(instance) != 0)
	{
		return -EIO;
	}

	flush_write(instance, path, desc);

	keep_alive_unlock(instance);

	return 0;
}

static int _rfs_write_cached(struct rfs_instance *instance, const char *path, const char *buf, size_t size, off_t offset, uint64_t desc)
{
	struct write_behind_request *write_behind_request = &instance->write_cache.write_behind_request;
	
	if (size > instance->write_cache.max_cache_size)
	{
		if (keep_alive_lock(instance) != 0)
		{
			return -EIO;
		}
		
		int flush_ret = flush_write(instance, path, desc);
		if (flush_ret < 0)
		{
			keep_alive_unlock(instance);
			return flush_ret;
		}
		
		int ret = 0;
		PARTIALY_DECORATE(ret, 
		_write, 
		instance, 
		path, 
		buf, 
		size, 
		offset, 
		desc);
		
		keep_alive_unlock(instance);
		return ret;
	}
	
	struct cache_block *block = find_suitable_cache_block(instance->write_cache.cache, size, offset, desc);
	DEBUG("cache block: %p\n", block);
	
	/* check if cache is full */
	if (block == NULL 
	&& cache_size(instance->write_cache.cache) >= instance->write_cache.max_cache_size)
	{
		if (keep_alive_lock(instance) != 0)
		{
			return -EIO;
		}
		
		int flush_ret = flush_write(instance, path, desc);
		if (flush_ret < 0)
		{
			keep_alive_unlock(instance);
			return flush_ret;
		}
		
		keep_alive_unlock(instance);
	}
	
	if (block == NULL  
	|| block->used >= block->allocated
	|| offset != block->offset + block->used) /* no suitable block exist yet */
	{
		if (block != NULL) /* block is found, but offset doesn't match required */
		{
			if (keep_alive_lock(instance) != 0)
			{
				return -EIO;
			}
			
			int flush_ret = flush_write(instance, path, desc);
			if (flush_ret < 0)
			{
				keep_alive_unlock(instance);
				return flush_ret;
			}
			
			keep_alive_unlock(instance);
		}
		
		/* try to reserve new block */
		block = NULL;
		block = reserve_cache_block(&instance->write_cache.cache, 
		instance->write_cache.max_cache_size / 2, 
		offset, desc);
	}
	
	size_t free_space = (block != NULL ? block->allocated - block->used : 0);
	
	while (1)
	{
	
	if (free_space >= size)
	{
		DEBUG("*** writing data to cache (%p)\n", block);
		memcpy(block->data + block->used, buf, size);
		block->used += size;
		
		if (block->used >= block->allocated)
		{
			/* no place left in existing block, fire write behind */
			
			if (keep_alive_lock(instance) != 0)
			{
				return -EIO;
			}
			
			if (write_behind_request->path != NULL)
			{
				free(write_behind_request->path);
				write_behind_request->path = NULL;
				if (write_behind_request->last_ret < 0)
				{
					keep_alive_unlock(instance);
					return write_behind_request->last_ret;
				}
			}
			
			write_behind_request->path = strdup(path);
			write_behind_request->block = block;
			
			if (rfs_sem_post(&instance->write_cache.write_behind_sem) != 0)
			{
				keep_alive_unlock(instance);
				return -EIO;
			}
			
			keep_alive_unlock(instance);
			
			/* ensure write behind is running */
			rfs_sem_wait(&instance->write_cache.write_behind_started);
		}
		
		return size;
	}
	
	break;
	}
	
	/* oops. we've missed cache */
	if (keep_alive_lock(instance) != 0)
	{
		return -EIO;
	}

	int flush_ret = flush_write(instance, path, desc);
	if (flush_ret < 0)
	{
		keep_alive_unlock(instance);
		return flush_ret;
	}
	
	int write_ret = 0;
	PARTIALY_DECORATE(write_ret, 
	_write, 
	instance, 
	path, 
	buf, 
	size, 
	offset, 
	desc);
	
	keep_alive_unlock(instance);
	return write_ret;
}

static int _write(struct rfs_instance *instance, const char *path, const char *buf, size_t size, off_t offset, uint64_t desc)
{
	struct write_behind_request *write_behind_request = &instance->write_cache.write_behind_request;
	
	if (write_behind_request->last_ret < 0
	&& write_behind_request->block != NULL
	&& write_behind_request->block->descriptor == desc)
	{
		int ret = write_behind_request->last_ret;
		
		delete_from_cache(instance, path);
		reset_write_behind(instance);
		return ret;
	}
	
	uint64_t handle = desc;
	uint64_t foffset = offset;
	uint32_t fsize = size;
	
	if (size < 1)
	{
		return 0;
	}

#define header_size sizeof(fsize) + sizeof(foffset) + sizeof(handle)
	unsigned overall_size = header_size + size;
	struct command cmd = { cmd_write, overall_size };

	char buffer[header_size] = { 0 };

	pack_64(&handle, buffer, 
	pack_64(&foffset, buffer, 
	pack_32(&fsize, buffer, 0
		)));

	if (rfs_send_cmd_data2(&instance->sendrecv, &cmd, buffer, header_size, buf, size) == -1)
	{
		return -ECONNABORTED;
	}
#undef header_size

	struct answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}

	if (ans.command != cmd_write || ans.data_len != 0)
	{
		return -EBADMSG;
	}
	
	delete_from_cache(instance, path);
	
	return ans.ret == -1 ? -ans.ret_errno : (int)ans.ret;
}

int _rfs_write(struct rfs_instance *instance, const char *path, const char *buf, size_t size, off_t offset, uint64_t desc)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}
	
	if (instance->config.use_write_cache != 0)
	{
		return _rfs_write_cached(instance, path, buf, size, offset, desc);
	}
	
	if (keep_alive_lock(instance) != 0)
	{
		return -EIO;
	}
	
	int ret = 0;
	PARTIALY_DECORATE(ret, 
	_write, 
	instance, 
	path, 
	buf, 
	size, 
	offset, 
	desc);
	
	/* clear read cache if any */
	clear_cache_by_desc(&instance->read_cache.cache, desc);
	
	keep_alive_unlock(instance);
	
	return ret;
}

