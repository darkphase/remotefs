/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <semaphore.h>

#include "data_cache.h"
#include "rfs_semaphore.h"
#include "resume.h"

static void* prefetch(void *void_instance);
static int _read(struct rfs_instance *instance, char *buf, size_t size, off_t offset, uint64_t desc);

static int init_prefetch(struct rfs_instance *instance)
{
	DEBUG("%s\n", "initing prefetch");
	
	instance->read_cache.prefetch_request.size = 0;
	instance->read_cache.prefetch_request.offset = (off_t)-1;
	instance->read_cache.prefetch_request.descriptor = -1;
	instance->read_cache.prefetch_request.please_die = 0;
	instance->read_cache.prefetch_request.inprogress = 0;

	DEBUG("%s\n", "initing semaphores");
	
	errno = 0;
	if (rfs_sem_init(&instance->read_cache.prefetch_sem, 0, 0) != 0)
	{
		return -errno;
	}
	
	DEBUG("%s\n", "creating prefetch thread");
	
	errno = 0;
	if (pthread_create(&instance->read_cache.prefetch_thread, NULL, prefetch, (void *)instance) != 0)
	{
		return -errno;
	}
	
	return 0;
}

static void kill_prefetch(struct rfs_instance *instance)
{
	DEBUG("%s\n", "killing prefetch");
	
	instance->read_cache.prefetch_request.please_die = 1;
	
	DEBUG("%s\n", "unlocking mutex");
	if (rfs_sem_post(&instance->read_cache.prefetch_sem) != 0)
	{
		return;
	}
	
	DEBUG("%s\n", "waiting for thread to end");
	pthread_join(instance->read_cache.prefetch_thread, NULL);
	rfs_sem_destroy(&instance->read_cache.prefetch_sem);
}

static void* prefetch(void *void_instance)
{
	struct rfs_instance *instance = (struct rfs_instance *)(void_instance);
	struct prefetch_request *prefetch_request = &instance->read_cache.prefetch_request;
	
	if (rfs_sem_init(&instance->read_cache.prefetch_started, 0, 0) != 0)
	{
		return NULL;
	}
	
	DEBUG("%s\n", "prefetch started. waiting for request");
	
	while (1)
	{
	
	if (rfs_sem_wait(&instance->read_cache.prefetch_sem) != 0)
	{
		pthread_exit(NULL);
	}
	
	if (prefetch_request->please_die != 0)
	{
		DEBUG("%s\n", "exiting prefetch");
		prefetch_request->inprogress = 0;
		pthread_exit(NULL);
	}
	
	if (keep_alive_lock(instance) != 0)
	{
		prefetch_request->inprogress = 0;
		pthread_exit(NULL);
	}
	
	rfs_sem_post(&instance->read_cache.prefetch_started);
	
	DEBUG("%s\n", "received prefetch request:");
	DEBUG("size: %lu, offset: %llu, descriptor: %llu\n", 
	(long unsigned int)prefetch_request->size,
	(unsigned long long)prefetch_request->offset,
	(unsigned long long)prefetch_request->descriptor);
	
	size_t prefetch_size = prefetch_request->size;
	off_t prefetch_offset = prefetch_request->offset + prefetch_request->size;
	uint64_t prefetch_descriptor = prefetch_request->descriptor;
	
	if (prefetch_size > instance->read_cache.max_cache_size)
	{
		prefetch_request->inprogress = 0;
		keep_alive_unlock(instance);
		continue;
	}
	
	struct list *open_rec = instance->resume.open_files;
	while (open_rec != NULL)
	{
		struct open_rec *info = (struct open_rec *)open_rec->data;
		if (info->desc == prefetch_descriptor)
		{
			if (info->last_used_read_block > prefetch_size
			&& info->last_used_read_block <= instance->read_cache.max_cache_size)
			{
				prefetch_size = info->last_used_read_block;
			}
			break;
		}
		
		open_rec = open_rec->next;
	}
	
	if (prefetch_size * 2 <= instance->read_cache.max_cache_size)
	{
		prefetch_size *= 2;
	}
	
	if (open_rec != NULL
	&& ((struct open_rec *)open_rec->data)->desc == prefetch_descriptor)
	{
		((struct open_rec *)open_rec->data)->last_used_read_block = prefetch_size;
	}
	
	struct cache_block *block = reserve_cache_block(
	&instance->read_cache.cache, 
	prefetch_size, 
	prefetch_offset,
	prefetch_descriptor);
	
	if (block == NULL)
	{
		prefetch_request->inprogress = 0;
		keep_alive_unlock(instance);
		continue;
	}

	DEBUG("*** prefetching %u bytes from offset %llu (%llu)\n", 
	(unsigned int)prefetch_size, 
	(unsigned long long)prefetch_offset,
	(unsigned long long)prefetch_descriptor);
	
	int ret = 0;
	PARTIALY_DECORATE(ret, 
	_read, 
	instance, 
	block->data, 
	prefetch_size, 
	prefetch_offset,
	prefetch_descriptor);
	
	if (ret < 0)
	{
		delete_block_from_cache(&instance->read_cache.cache, block);
		
		prefetch_request->inprogress = 0;
		keep_alive_unlock(instance);
		continue;
	}
	
	block->used = ret;
	
#ifdef RFS_DEBUG
	dump_block(block);
#endif
	
	prefetch_request->inprogress = 0;
	keep_alive_unlock(instance);
	
	} /* while (1) */
#if defined QNX
	return NULL; /* gcc error ! */
#endif
}

static int _rfs_read_cached(struct rfs_instance *instance, const char *path, char *buf, size_t size, off_t offset, uint64_t desc)
{
	struct prefetch_request *prefetch_request = &instance->read_cache.prefetch_request;

	prefetch_request->inprogress = 0;
	
	struct cache_block *block = find_suitable_cache_block(instance->read_cache.cache, size, offset, desc);
	
	DEBUG("cache block: %p\n", block);

	int ret = 0;
	
	if (block == NULL
	|| block->used == 0) /* prefetch may be still in progress */
	{
		DEBUG("prefetch progress: %u, desc: %llu\n", 
		prefetch_request->inprogress,
		(unsigned long long)prefetch_request->descriptor);
		
		/* if so, then wait for it to finish */
		if (prefetch_request->descriptor == desc)
		{
			DEBUG("%s\n", "waiting for prefetch to finish");
			if (keep_alive_lock(instance) != 0)
			{
				return -EIO;
			}
			
			block = find_suitable_cache_block(instance->read_cache.cache, size, offset, desc);
			
			keep_alive_unlock(instance);
		}
	}
	
	if (block != NULL
	&& (block->used + block->offset >= size + offset))
	{
		size_t cached_size = (block->offset + block->used) - offset;
		DEBUG("*** hit (%d)\n", (int)cached_size);
		const char *cached_data = block->data + (offset - block->offset);
		
		memcpy(buf, cached_data, size);
		ret = size;
		
		if (cached_size > size)
		{
			return ret; /* no prefetch needed */
		}
	}
	else /* we're missed cache, so whatever, read as always and try
	to prefetch next cache block more precisely */
	{
		DEBUG("*** miss (offset: %llu, size: %lu)\n", offset, (unsigned long)size);
		
		if (keep_alive_lock(instance) != 0)
		{
			return -EIO;
		}
		
		ret = 0;
		PARTIALY_DECORATE(ret, 
		_read, 
		instance, 
		buf, 
		size, 
		offset, 
		desc);
		
		keep_alive_unlock(instance);
		
		if (ret < 0)
		{
			/* no prefetch on error */
			return ret;
		}
	}
	
	/* if we're here, then we missed cache and already have requested data
	or cache become empty after current read, but we still have requested data
	
	now we need to read-ahead some more cache */
	if (ret == size 
	&& size >= PREFETCH_LIMIT)
	{
		if (keep_alive_lock(instance) != 0)
		{
			return -EIO;
		}
		
		clear_cache_by_desc(&instance->read_cache.cache, desc);
		
		prefetch_request->size = size;
		prefetch_request->offset = offset;
		prefetch_request->descriptor = desc;
		prefetch_request->inprogress = 1;
		
		if (rfs_sem_post(&instance->read_cache.prefetch_sem) != 0)
		{
			keep_alive_unlock(instance);
			prefetch_request->inprogress = 0;
			return -EIO;
		}
		
		keep_alive_unlock(instance);
		
		/* ensure prefetch is running */
		rfs_sem_wait(&instance->read_cache.prefetch_started);
		
		return ret;
	}
	
	/* we actualy ignoring prefetch return value because it has nothing
	to do with current read operation, which is already completed */
	
	return ret;
}

static int _read(struct rfs_instance *instance, char *buf, size_t size, off_t offset, uint64_t desc)
{
	DEBUG("*** reading %lu bytes at %llu\n", (unsigned long)size, offset);
	
	instance->sendrecv.oob_received = 0;
	
	uint64_t handle = desc;
	uint64_t foffset = offset;
	uint32_t fsize = size;

#define overall_size sizeof(fsize) + sizeof(foffset) + sizeof(handle)
	struct command cmd = { cmd_read, overall_size };

	char buffer[overall_size] = { 0 };
#undef  overall_size

	pack_64(&handle, buffer, 
	pack_64(&foffset, buffer, 
	pack_32(&fsize, buffer, 0
		)));

	if (rfs_send_cmd_data(&instance->sendrecv, &cmd, buffer, cmd.data_len) == -1)
	{
		return -ECONNABORTED;
	}

	struct answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}

	if (ans.command != cmd_read || ans.data_len > size)
	{
		rfs_ignore_incoming_data(&instance->sendrecv, ans.data_len);
		return -EBADMSG;
	}

	if (ans.ret < 0)
	{
		return -ans.ret_errno;
	}

	if (ans.data_len > 0)
	{
		if (rfs_receive_data(&instance->sendrecv, buf, ans.data_len) == -1)
		{
			return -ECONNABORTED;
		}
	}
	
	if (instance->sendrecv.oob_received != 0)
	{
		if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
		{
			return -ECONNABORTED;
		}
		
		if (ans.command != cmd_read || ans.data_len > size)
		{
			return -EBADMSG;
		}
		
		if (ans.ret < 0)
		{
			return -ans.ret_errno;
		}
		
		return ans.ret;
	}

	return ans.data_len;
}

static int _rfs_read(struct rfs_instance *instance, const char *path, char *buf, size_t size, off_t offset, uint64_t desc)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}
	
	if (instance->config.use_read_cache)
	{
		/* flush this file first */
		if (keep_alive_lock(instance) != 0)
		{
			return -EIO;
		}
		
		flush_write(instance, path, desc);
		
		keep_alive_unlock(instance);
		
		return _rfs_read_cached(instance, path, buf, size, offset, desc);
	}
	else
	{
		if (keep_alive_lock(instance) != 0)
		{
			return -EIO;
		}
		
		int ret = 0;
		PARTIALY_DECORATE(ret, _read, instance, buf, size, offset, desc);
		
		keep_alive_unlock(instance);
		return ret;
	}
}
