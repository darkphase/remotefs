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
#include "../keep_alive_client.h"
#include "../list.h"
#include "../psemaphore.h"
#include "../sendrecv_client.h"
#include "operations.h"
#include "utils.h"

static int _rfs_flush_write(struct rfs_instance *instance, const char *path, uint64_t desc)
{
	if (client_keep_alive_lock(instance) != 0)
	{
		return -EIO;
	}

	int ret = _flush_write(instance, path, desc);
		
	client_keep_alive_unlock(instance);

	return ret;
}

static int _rfs_write_missed_cache(struct rfs_instance *instance, const char *path, const char *buf, size_t size, off_t offset, uint64_t desc)
{
	if (client_keep_alive_lock(instance) != 0)
	{
		return -EIO;
	}
		
	int flush_ret = _flush_write(instance, path, desc);
	if (flush_ret < 0)
	{
		client_keep_alive_unlock(instance);
		return flush_ret;
	}
		
	int ret = 0;
	PARTIALY_DECORATE(ret, 
	_do_write, 
	instance, 
	path, 
	buf, 
	size, 
	offset, 
	desc);
		
	client_keep_alive_unlock(instance);
	return ret;
}

static int _rfs_write_cached(struct rfs_instance *instance, const char *path, const char *buf, size_t size, off_t offset, uint64_t desc)
{
	if (size > instance->write_cache.max_cache_size)
	{
		return _rfs_write_missed_cache(instance, path, buf, size, offset, desc);
	}
	
	int in_cache = file_in_cache(instance->write_cache.cache, desc);

	struct cache_block *block = NULL;

	if (in_cache != 0)
	{
		block = find_suitable_cache_block(instance->write_cache.cache, size, offset, desc);
	}
	else
	{
		block = reserve_cache_block(&instance->write_cache.cache,
				instance->write_cache.max_cache_size / 2,
				offset,
				desc);
	}

	DEBUG("cache block: %p\n", (void *)block);
	
	if (block == NULL)
	{
		/* file is in cache, but block is not suitable for this write
		flush file since write order matters */
		if (in_cache)
		{
			int flush_ret = _rfs_flush_write(instance, path, desc);
			if (flush_ret < 0)
			{
				return flush_ret;
			}
		}

		return _rfs_write_missed_cache(instance, path, buf, size, offset, desc);
	}

	DEBUG("*** writing data to cache (%p)\n", (void *)block);
	memcpy(block->data + block->used, buf, size);
	block->used += size;

	DEBUG("*** used: %d, allocated: %d\n", block->used, block->allocated);
		
	if (block->used >= block->allocated)
	{
		int flush_ret = _rfs_flush_write(instance, path, desc);
		if (flush_ret < 0)
		{
			return flush_ret;
		}
	}

	return size;
}

int _do_write(struct rfs_instance *instance, const char *path, const char *buf, size_t size, off_t offset, uint64_t desc)
{
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

	char header[header_size] = { 0 };

	pack_64(&handle, 
	pack_64(&foffset, 
	pack_32(&fsize, header
	)));

	send_token_t token = { 0 };
	if (do_send(&instance->sendrecv, 
		queue_data(buf, size, 
		queue_data(header, header_size, 
		queue_cmd(&cmd, &token 
		)))) < 0)
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
	
	delete_from_cache(&instance->attr_cache, path);
	
	return (ans.ret < 0 ? -ans.ret_errno : (int)ans.ret);
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
	
	if (client_keep_alive_lock(instance) != 0)
	{
		return -EIO;
	}
	
	int ret = 0;
	PARTIALY_DECORATE(ret, 
	_do_write, 
	instance, 
	path, 
	buf, 
	size, 
	offset, 
	desc);
	
	client_keep_alive_unlock(instance);
	
	return ret;
}
