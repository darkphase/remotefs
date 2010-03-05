/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_WRITE_H
#define OPERATIONS_WRITE_H

/** rfs write operation(s) */

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/** write-behind control struct */
struct write_behind_request
{
	struct cache_block *block;
	unsigned int please_die;
	int last_ret;
	char *path;
};

struct rfs_instance;

/** init write-behind control info */
int init_write_behind(struct rfs_instance *instance);

/** stop write-behind thread */
void kill_write_behind(struct rfs_instance *instance);

void reset_write_behind(struct rfs_instance *instance);

int _write(struct rfs_instance *instance, const char *path, const char *buf, size_t size, off_t offset, uint64_t desc);
int _rfs_write(struct rfs_instance *instance, const char *path, const char *buf, size_t size, off_t offset, uint64_t desc);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* OPERATIONS_WRITE_H */
