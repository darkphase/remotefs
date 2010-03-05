/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_READ_H
#define OPERATIONS_READ_H

/** read */

#include <stdint.h>
#include <sys/types.h>

struct rfs_instance;
int _rfs_read(struct rfs_instance *instance, const char *path, char *buf, size_t size, off_t offset, uint64_t desc);

#endif /* OPERATIONS_READ_H */
