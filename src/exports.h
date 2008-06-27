#ifndef EXPORTS_H
#define EXPORTS_H

#include <sys/types.h>

#include "config.h"

struct list;

struct rfs_export
{
	char *path;
	struct list *users;
	unsigned options;
	uid_t export_uid;
};

enum e_export_opts { opt_ro = 1 };

unsigned parse_exports();
void release_exports();

const struct rfs_export* get_export(const char *path);
unsigned is_ipaddr(const char *string);

void dump_exports();

#endif // EXPORTS_H
