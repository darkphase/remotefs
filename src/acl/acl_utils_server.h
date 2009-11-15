/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

/** server's ACL utilities */

#include "../options.h"

#ifdef ACL_AVAILABLE

#ifndef RFS_ACL_UTILS_SERVER_H
#define RFS_ACL_UTILS_SERVER_H

# if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
# endif

#include <stdint.h>
#include <sys/acl.h>

uint32_t id_lookup_resolve(acl_tag_t tag, const char *name, size_t name_len, void *lookup_casted);
char* id_lookup_reverse_resolve(acl_tag_t tag, void *id, void *lookup_casted);

int rfs_get_file_acl(const char *path, const char *acl_name, acl_t *acl);
int rfs_set_file_acl(const char *path, const char *acl_name, const acl_t acl);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* RFS_ACL_UTILS_SERVER_H */

#endif /* ACL_AVAILABLE */

