/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

/** resolving of names using id lookup tables */

#include "../options.h"

#ifdef ACL_AVAILABLE

#ifndef RFS_ID_LOOKUP_RESOLVE_H
#define RFS_ID_LOOKUP_RESOLVE_H

# if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
# endif

#include <stdint.h>
#include <sys/acl.h>

uint32_t id_lookup_resolve(acl_tag_t tag, const char *name, size_t name_len, void *lookup_casted);
char* id_lookup_reverse_resolve(acl_tag_t tag, void *id, void *lookup_casted);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* RFS_ID_LOOKUP_RESOLVE_H */

#endif /* ACL_AVAILABLE */

