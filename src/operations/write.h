/*
remotefs file system
See the file AUTHORS for copyright information.

This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_WRITE_H
#define OPERATIONS_WRITE_H

/** rfs write operation(s) */

#include "../config.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/** write-behind cache block */
typedef struct
{
	size_t allocated;
	size_t used;
	uint64_t descriptor;
	off_t offset;
	char data[DEFAULT_RW_CACHE_SIZE / 2];
	char *path;
} rfs_write_cache_block_t;

struct rfs_instance;

/** init write-behind control info */
int init_write_behind(struct rfs_instance *instance);

/** stop write-behind thread */
void kill_write_behind(struct rfs_instance *instance);

/** reset write cache block */
void reset_write_cache_block(rfs_write_cache_block_t *block);

int _do_write(struct rfs_instance *instance, const char *path, const char *buf, size_t size, off_t offset, uint64_t desc);
int _rfs_write(struct rfs_instance *instance, const char *path, const char *buf, size_t size, off_t offset, uint64_t desc);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* OPERATIONS_WRITE_H */
