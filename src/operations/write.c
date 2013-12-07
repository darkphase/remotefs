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
#include "../instance_client.h"
#include "../keep_alive_client.h"
#include "../list.h"
#include "../psemaphore.h"
#include "../sendrecv_client.h"
#include "operations.h"
#include "utils.h"

static void* write_behind(void *void_instance);

void reset_write_cache_block(rfs_write_cache_block_t *block)
{
	block->allocated = sizeof(block->data) * sizeof(*(block->data));
	block->used = 0;
	block->descriptor = (uint64_t)(-1);
	block->offset = 0;

	if (block->path != NULL)
	{
		free(block->path);
	}

	block->path = NULL;
}

int init_write_behind(struct rfs_instance *instance)
{
	DEBUG("%s\n", "initing write behind");

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

	instance->write_cache.please_die = 1;

	DEBUG("%s\n", "unlocking mutex");
	if (rfs_sem_post(&instance->write_cache.write_behind_sem) != 0)
	{
		return;
	}

	DEBUG("%s\n", "waiting for thread to end");
	pthread_join(instance->write_cache.write_behind_thread, NULL);
	rfs_sem_destroy(&instance->write_cache.write_behind_sem);
}

static void* write_behind(void *void_instance)
{
	struct rfs_instance *instance = (struct rfs_instance *)(void_instance);

	if (rfs_sem_init(&instance->write_cache.write_behind_started, 0, 0) != 0)
	{
		return NULL;
	}

	DEBUG("%s\n", "write behind started. waiting for request");

	while (1)
	{

	DEBUG("%s\n", "*** write behind iterating");

	if (rfs_sem_wait(&instance->write_cache.write_behind_sem) != 0)
	{
		pthread_exit(NULL);
	}

	DEBUG("%s\n", "*** received semaphore event");

	if (instance->write_cache.please_die != 0)
	{
		DEBUG("%s\n", "exiting write behind");
		pthread_exit(NULL);
	}

	/* need to lock keep alive to avoid concurrent flush() and so
	 * rfs_write() won't check this lock until actual write is required */
	if (client_keep_alive_lock(instance) != 0)
	{
		instance->write_cache.last_ret = -EIO;
		pthread_exit(NULL);
	}

	DEBUG("%s\n", "*** write behind started");

	rfs_write_cache_block_t *block = instance->write_cache.current_block;

	/* switch current block, so writes can proceed with it */
	instance->write_cache.current_block =
		(instance->write_cache.current_block == &instance->write_cache.block1
		? &instance->write_cache.block2
		: &instance->write_cache.block1);

	rfs_sem_post(&instance->write_cache.write_behind_started);

	if (block->used <= 0) /* oops. was it already flush'ed? */
	{
		DEBUG("%s\n", "*** write behind finished");
		client_keep_alive_unlock(instance);
		continue;
	}

	instance->write_cache.last_ret = 0;

	DEBUG("%s\n", "received write behind request:");
	DEBUG("size: %u, offset: %llu, descriptor: %llu\n",
		(unsigned int )block->used,
		(unsigned long long)block->offset,
		(unsigned long long)block->descriptor);

	instance->write_cache.last_ret = 0;
	PARTIALY_DECORATE(instance->write_cache.last_ret,
		_do_write,
		instance,
		block->path,
		block->data,
		block->used,
		block->offset,
		block->descriptor);

	reset_write_cache_block(block);

	DEBUG("%s\n", "*** write behind finished");

	client_keep_alive_unlock(instance);

	} /* while (1) */

	pthread_exit(NULL);
}

static int _rfs_flush_write(struct rfs_instance *instance, const char *path, uint64_t desc)
{
	if (client_keep_alive_lock(instance) != 0)
	{
		return -EIO;
	}

	int ret = _flush_write(instance, path, desc);
	if (ret < 0)
	{
		client_keep_alive_unlock(instance);
		return ret;
	}

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
	/* if requested block doesn't fit cache under any condiftion
	 * rfs_flush() always need to be called before proceeding with writes
	 * write order matters and should match requests order */

	rfs_write_cache_block_t *current_block = instance->write_cache.current_block;

	/* missed bad, leave current cache as is
	 * this will let original file to be cached properly at least */
	if (current_block->descriptor != (uint64_t)(-1)
	&& current_block->descriptor != desc)
	{
		return _rfs_write_missed_cache(instance, path, buf, size, offset, desc);
	}

	/* won't fit cache anyway
	 * normally this would never happen, but still need to be here */
	if (size > current_block->allocated)
	{
		int flush_ret = _rfs_flush_write(instance,
			current_block->path, current_block->descriptor);

		if (flush_ret < 0)
		{
			return flush_ret;
		}

		return _rfs_write_missed_cache(instance, path, buf, size, offset, desc);
	}

	/* check if cache offset match requested offset
	 * and requested size will fit into remaining block size */
	if (current_block->descriptor == desc
	&& (current_block->used >= current_block->allocated
		|| current_block->used + size >= current_block->allocated
		|| offset != current_block->offset + current_block->used))
	{
		int flush_ret = _rfs_flush_write(instance,
			current_block->path, current_block->descriptor);

		if (flush_ret < 0)
		{
			return flush_ret;
		}
	}

	/* if cache was flushed, current_block should point to "clean", ready-to-use block */

	if (current_block->descriptor == (uint64_t)(-1))
	{
		current_block->descriptor = desc;
		current_block->offset = offset;
		current_block->path = strdup(path);
	}

	DEBUG("*** writing data to cache (%p)\n", (void *)(current_block));
	DEBUG("*** cache desc: %llu\n", (long long unsigned)(current_block->descriptor));
	DEBUG("*** block size: %llu, block offset: %llu\n",
		(long long unsigned)(size), (long long unsigned)(offset));

	memcpy(current_block->data + current_block->used, buf, size);
	current_block->used += size;

	if (current_block->used >= current_block->allocated)
	{
		/* no place left in existing block after write - fire write behind */

		if (client_keep_alive_lock(instance) != 0)
		{
			return -EIO;
		}

		/* write behind is always picking instance->write_cache.current_block */
		if (rfs_sem_post(&instance->write_cache.write_behind_sem) != 0)
		{
			reset_write_cache_block(current_block);
			client_keep_alive_unlock(instance);
			return -EIO;
		}

		client_keep_alive_unlock(instance);

		/* ensure write behind is running before proceeding*/
		rfs_sem_wait(&instance->write_cache.write_behind_started);
	}

	return size;
}

int _do_write(struct rfs_instance *instance, const char *path, const char *buf, size_t size, off_t offset, uint64_t desc)
{
	if (instance->write_cache.last_ret < 0)
	{
		int ret = instance->write_cache.last_ret;
		delete_from_cache(&instance->attr_cache, path);

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
	struct rfs_command cmd = { cmd_write, overall_size };

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

	struct rfs_answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}

	if (ans.command != cmd_write || ans.data_len != 0)
	{
		return -EBADMSG;
	}

	delete_from_cache(&instance->attr_cache, path);

	return ans.ret == -1 ? -ans.ret_errno : (int)ans.ret;
}

int _rfs_write(struct rfs_instance *instance, const char *path, const char *buf, size_t size, off_t offset, uint64_t desc)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	return _rfs_write_cached(instance, path, buf, size, offset, desc);
}
