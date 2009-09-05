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
#include <sys/types.h>
#include <sys/stat.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct rfs_instance;

/** convert OS file flags (O_RDWR, etc) to rfs flags (RFS_RDWR, etc) */
uint16_t rfs_file_flags(int os_flags);

/** unpack stat block from buffer */
const char* unpack_stat(struct stat *result, const char *buffer);

uid_t resolve_username(struct rfs_instance *instance, const char *user);
gid_t resolve_groupname(struct rfs_instance *instance, const char *group, const char *user);

#endif /* OPERATIONS_UTILS_H */

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif
