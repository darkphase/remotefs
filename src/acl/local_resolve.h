/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

/** resolving of names using libc */

#include "../options.h"

#ifdef ACL_OPERATIONS_AVAILABLE

#ifndef RFS_LOCAL_RESOLVE_H
#define RFS_LOCAL_RESOLVE_H

# if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
# endif

#include <stdint.h>
#include <sys/acl.h>

uint32_t local_resolve(acl_tag_t tag, const char *name, size_t name_len, void *instance_casted);
char* local_reverse_resolve(acl_tag_t tag, void *id, void *instance_casted);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* RFS_LOCAL_RESOLVE_H */

#endif /* ACL_OPERATIONS_AVAILABLE */
