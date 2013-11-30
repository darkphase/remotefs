/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef RESOLVE_H
#define RESOLVE_H

/** resolving */

# if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
# endif

struct rfs_list;

struct resolved_addr
{
	char *ip;
	int addr_family;
};

/** don't forget to destroy_list() returned value */
struct rfs_list* host_ips(const char *host, int *address_family);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* RESOLVE_H */
