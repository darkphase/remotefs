/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

/** server's ACL utilities */

#include "options.h"

#ifdef ACL_AVAILABLE

#ifndef RFS_ACL_UTILS_SERVER_H
#define RFS_ACL_UTILS_SERVER_H

# if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
# endif

#include <stdint.h>

uint32_t id_lookup_resolve(uint16_t type, const char *name, size_t name_len, void *lookup_casted);
char* id_lookup_reverse_resolve(uint16_t type, uint32_t id, void *lookup_casted);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* RFS_ACL_UTILS_SERVER_H */

#endif /* ACL_AVAILABLE */

