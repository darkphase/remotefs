/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

/** ACL utilities */

#include "options.h"

#ifdef ACL_AVAILABLE

#ifndef RFS_ACL_H
#define RFS_ACL_H

# if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
# endif

#include <sys/types.h>
#include <stdint.h>

#include "acl/include/acl_ea.h"
#include "acl/include/acl.h"

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

typedef uint32_t (*resolve)(uint16_t type, const char *name, size_t name_len, void *data);
typedef char* (*reverse_resolve)(uint16_t type, uint32_t id, void *data);

/* don't forget to free_buffer() result */
rfs_acl_t* rfs_acl_from_text(const struct id_lookup_info *lookup, 
	const char *text,
	resolve custom_resolve, 
	void *custom_resolve_data, 
	size_t *count);

/* don't forget to free_buffer() result */
char* rfs_acl_to_text(const struct id_lookup_info *lookup, 
	const rfs_acl_t *acl, 
	size_t count, 
	reverse_resolve custom_resolve, 
	void *custom_resolve_data, 
	size_t *len);

typedef int (*walk_acl_callback)(uint16_t type, uint16_t perm, uint32_t id, void *data);
typedef int (*walk_acl_text_callback)(uint16_t type, uint16_t perm, const char *name, size_t name_len, void *data);

/* walk ACL record and call callback on each entry */
int walk_acl(const rfs_acl_t *acl, size_t count, walk_acl_callback callback, void *data);

/* walk ACL text and call callback on each entry */
int walk_acl_text(const char *acl_text, walk_acl_text_callback callback, void *data);

#ifdef RFS_DEBUG
void dump_acl(const struct id_lookup_info *lookup, const rfs_acl_t *acl, int count);
#endif

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* RFS_ACL_H */

#endif /* ACL_AVAILABLE */

