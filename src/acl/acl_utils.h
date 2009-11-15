/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

/** ACL utilities */

#include "../options.h"

#ifdef ACL_AVAILABLE

#ifndef RFS_ACL_UTILS_H
#define RFS_ACL_UTILS_H

# if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
# endif

#include <stdint.h>
#include <sys/types.h>
#include <sys/acl.h>

#define STR_USER_TAG         "u:"
#define STR_GROUP_TAG        "g:"
#define STR_MASK_TAG         "m:"
#define STR_OTHER_TAG        "o:"
#define STR_ACL_DELIMITER    ":"
#define STR_ACL_RWX          "rwx"
#define STR_UNKNOWN_TAG      "@"

typedef uint32_t (*resolve)(acl_tag_t tag, const char *name, size_t name_len, void *data);
typedef char* (*reverse_resolve)(acl_tag_t tag, void *id, void *data);

/* don't forget to acl_free result */
acl_t rfs_acl_from_text(const char *text,
	resolve custom_resolve, 
	void *custom_resolve_data);

/* don't forget to free_buffer result */
char* rfs_acl_to_text(const acl_t acl, 
	reverse_resolve custom_resolve, 
	void *custom_resolve_data, 
	size_t *len);

typedef int (*walk_acl_callback)(acl_tag_t tag, int perms, void *id, void *data);
typedef int (*walk_acl_text_callback)(acl_tag_t tag, int perms, const char *name, size_t name_len, void *data);

/* walk ACL record and call callback on each entry */
int walk_acl(const acl_t acl, walk_acl_callback callback, void *data);

/* walk ACL text and call callback on each entry */
int walk_acl_text(const char *acl_text, walk_acl_text_callback callback, void *data);

#ifdef RFS_DEBUG
void dump_acl(const acl_t acl);
#endif

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* RFS_ACL_UTILS_H */
#endif /* ACL_AVAILABLE */

