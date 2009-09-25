/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef EXPORTS_H
#define EXPORTS_H

/** exports file routines */

#include <sys/types.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct list;

/** user information
prefix_len is optional and set for ip-addresses only */
struct user_rec
{
	char *id;
	unsigned prefix_len;
};

/** export information */
struct rfs_export
{
	char *path;
	struct list *users;
	unsigned options;
	uid_t export_uid;
};

/** parse exports_file to exports
@uid is a value set by rfsd -u to use by default  (if no uid is set for specific export)
@return 0 on success, -errno on system error, line number on parsing error */
int parse_exports(const char *exports_file, struct list **exports, uid_t worker_uid);

/** free memory allocated for exports */
void release_exports(struct list **exports);

/** get export info by export path */
const struct rfs_export* get_export(const struct list *exports, const char *path);

#ifdef RFS_DEBUG
extern void dump_exports(const struct list *exports);
#endif

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* EXPORTS_H */

