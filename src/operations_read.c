/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <string.h>

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
#ifdef RFS_DEBUG
#	include "ratio.h"
#endif
#include "resume.h"
#include "sendrecv.h"

static int _read(struct rfs_instance *instance, char *buf, size_t size, off_t offset, uint64_t desc);

static int prefetch(struct rfs_instance *instance, uint64_t desc, off_t offset, size_t size)
{
	size_t prefetch_size = size;
	
	if (prefetch_size > instance->read_cache.max_cache_size)
	{
		return -E2BIG;
	}
	
	struct list *open_rec = instance->resume.open_files;
	while (open_rec != NULL)
	{
		struct open_rec *info = (struct open_rec *)open_rec->data;
		if (info->desc == desc)
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
	
	struct cache_block *block = reserve_cache_block(
	&instance->read_cache.cache, 
	prefetch_size, 
	offset,
	desc);
	
	if (block == NULL)
	{
		return -EIO;
	}

	DEBUG("*** prefetching %u bytes from offset %llu (%llu)\n", 
	(unsigned int)prefetch_size, 
	(unsigned long long)offset,
	(unsigned long long)desc);
	
	int ret = 0;
	PARTIALY_DECORATE(ret, 
	_read, 
	instance, 
	block->data, 
	prefetch_size, 
	offset,
	desc);
	
	if (ret < 0)
	{
		delete_block_from_cache(&instance->read_cache.cache, block);

		return -EIO;
	}

	if (open_rec != NULL
	&& ((struct open_rec *)open_rec->data)->desc == desc)
	{
		((struct open_rec *)open_rec->data)->last_used_read_block = prefetch_size;
	}

	block->used = ret;
	
#ifdef RFS_DEBUG
	dump_block(block);
#endif

	return ret;
}

static int _rfs_read_cached(struct rfs_instance *instance, const char *path, char *buf, size_t size, off_t offset, uint64_t desc)
{
	struct cache_block *block = find_suitable_cache_block(instance->read_cache.cache, size, offset, desc);
	
	DEBUG("cache block: %p\n", block);

	int ret = 0;
	
	if (block != NULL
	&& (block->used + block->offset >= size + offset
		|| block->used < block->allocated)) /* this means that EOF was already reached */
	{
		size_t cached_size = (block->offset + block->used) - offset;
		DEBUG("*** hit (%d)\n", (int)cached_size);
#ifdef RFS_DEBUG
		++instance->read_cache.hits;
#endif
		const char *cached_data = block->data + (offset - block->offset);
		
		memcpy(buf, cached_data, size);
		ret = size;
		
		if (cached_size > size
		|| block->used < block->allocated)
		{
			return ret; /* no prefetch needed */
		}
	}
	else /* we're missed cache, so whatever, read as always and try
	to prefetch next cache block more precisely */
	{
		DEBUG("*** miss (offset: %llu, size: %lu)\n", (unsigned long long)offset, (unsigned long)size);
#ifdef RFS_DEBUG
		++instance->read_cache.misses;
#endif
		
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
	
	/* if we're here, then we missed cache, but already have requested data
	or cache become empty after current read, and we need to prefetch next read block, still we already have requested data

	so, in short: now we need to read-ahead some more cache from offset + size */
	if (ret == size 
	&& size >= PREFETCH_LIMIT)
	{
		if (keep_alive_lock(instance) != 0)
		{
			return -EIO;
		}
		
		clear_cache_by_desc(&instance->read_cache.cache, desc);
		
		prefetch(instance, desc, offset + size, size);
		
		keep_alive_unlock(instance);
		
		return ret;
	}
	
	/* we actualy ignoring prefetch return value because it has nothing
	to do with current read operation, which is already completed */
	
	return ret;
}

static int _read(struct rfs_instance *instance, char *buf, size_t size, off_t offset, uint64_t desc)
{
	DEBUG("*** reading %lu bytes at %llu\n", (unsigned long)size, (unsigned long long)offset);
	
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

int _rfs_read(struct rfs_instance *instance, const char *path, char *buf, size_t size, off_t offset, uint64_t desc)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}
	
	if (instance->config.use_read_cache > 0) 
	/* "> 0" matters. it's -1 by default (off) 
	and 0 if user turned it off with cmd-line parameter 
	so it shouldn't be "!= 0" */
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

#ifdef RFS_DEBUG
void dump_read_cache_stats(struct rfs_instance *instance)
{
	DEBUG("read cache hits: %lu, misses: %lu, ratio: %.2f\n", 
	instance->read_cache.hits, 
	instance->read_cache.misses, 
	ratio(instance->read_cache.hits, instance->read_cache.misses));
}
#endif

