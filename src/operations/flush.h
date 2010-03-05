/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_FLUSH_H
#define OPERATIONS_FLUSH_H

#include <stdint.h>

/** flush */

struct rfs_instance;
int _rfs_flush(struct rfs_instance *instance, const char *path, uint64_t desc);
int _flush_write(struct rfs_instance *instance, const char *path, uint64_t desc);

#endif /* OPERATIONS_FLUSH_H */

