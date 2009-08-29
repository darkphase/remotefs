/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

/** various routines for operations */

#ifndef OPERATIONS_UTILS_H
#define OPERATIONS_UTILS_H

#include <stdint.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct rfs_instance;

/** convert OS file flags (O_RDWR, etc) to rfs flags (RFS_RDWR, etc) */
uint16_t rfs_file_flags(int os_flags);

/** return size of rfs' stat block */
size_t stat_size();

/** unpack stat block from buffer */
off_t unpack_stat(struct rfs_instance *instance, const char *buffer, struct stat *result, int *ret);

#endif /* OPERATIONS_UTILS_H */

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif
