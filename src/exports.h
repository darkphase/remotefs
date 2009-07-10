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

/** export information */
struct rfs_export
{
	char *path;
	struct list *users;
	unsigned options;
	uid_t export_uid;
};

/** parse exports files. data will be stored in static variable
@see exports.c 
@return 0 on success, -errno on system error, line number on parsing error
*/
int parse_exports(const char *exports_file, struct list **exports, uid_t worker_uid);

/** delete parsed exports info and free memory allocated for exports */
void release_exports(struct list **exports);

/** get export info by path */
const struct rfs_export* get_export(const struct list *exports, const char *path);

#ifdef RFS_DEBUG
/** write exports to output. debug only */
extern void dump_exports(const struct list *exports);
#endif

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* EXPORTS_H */

