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
#include "../list.h"
#include "../sendrecv_client.h"
#include "operations.h"
#include "utils.h"
#include "write.h"

int _flush_write(struct rfs_instance *instance, const char *path, uint64_t desc)
{
	DEBUG("flushing file %llu\n", (unsigned long long)desc);

	struct rfs_write_cache_block * current_block = instance->write_cache.current_block;

	if (current_block->descriptor == desc)
	{
		int ret = 0;
		PARTIALY_DECORATE(ret,
			_do_write,
			instance,
			path,
			current_block->data,
			current_block->used,
			current_block->offset,
			current_block->descriptor);

		reset_write_cache_block(current_block);

		if (ret < 0)
		{
			return ret;
		}
	}

	return 0;
}

int _rfs_flush(struct rfs_instance *instance, const char *path, uint64_t desc)
{
	return _flush_write(instance, path, desc);
}
