/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

/** ACL utilities */

#ifndef RFS_ACL_H
#define RFS_ACL_H

#ifdef WITH_ACL

# if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
# endif

#include <sys/types.h>
#include "acl/include/acl_ea.h"

#define STR_USER_TAG         "u:"
#define STR_GROUP_TAG        "g:"
#define STR_MASK_TAG         "m:"
#define STR_OTHER_TAG        "o:"
#define STR_ACL_DELIMITER    ":"
#define STR_ACL_RWX          "rwx"
#define STR_UNKNOWN_TAG      "@"

typedef acl_ea_header rfs_acl_t;
typedef acl_ea_entry rfs_acl_entry_t;

struct list;
struct id_lookup_info;

/* don't forget to free_buffer() result */
rfs_acl_t* rfs_acl_from_xattr(const char *value, size_t size);

/* don't forget to free_buffer() result */
char* rfs_acl_to_xattr(const rfs_acl_t *acl, int count);

/* don't forget to free_buffer() result */
rfs_acl_t* rfs_acl_from_text(struct id_lookup_info *lookup, 
	const char *text,
	int *count);

/* don't forget to free_buffer() result */
char* rfs_acl_to_text(struct id_lookup_info *lookup, 
	const rfs_acl_t *acl, 
	int count, 
	size_t *len);

#ifdef RFS_DEBUG
void dump_acl(struct id_lookup_info *lookup, const rfs_acl_t *acl, int count);
#endif

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* WITH_ACL */

#endif /* RFS_ACL_H */

