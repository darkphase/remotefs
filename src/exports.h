#ifndef EXPORTS_H
#define EXPORTS_H

#include <sys/types.h>

#include "config.h"

/* exports file routines */

struct list;

/** export information */
struct rfs_export
{
	char *path;
	struct list *users;
	unsigned options;
	uid_t export_uid;
};

/** on/off export options */
enum e_export_opts { opt_ro = 1 };

/** parse exports files. data will be stored in static variable
@see exports.c 
*/
unsigned parse_exports();

/** delete parsed exports info and free memory allocated for exports */
void release_exports();

/** get export info by path */
const struct rfs_export* get_export(const char *path);

/** check if specified user is actually an ip address */
unsigned is_ipaddr(const char *string);

/** write exports to output. debug only */
void dump_exports();

#endif // EXPORTS_H
