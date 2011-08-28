/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef UTILS_H
#define UTILS_H

/** various utils */

#include "exports.h"

struct sockaddr_in;
struct sockaddr_in6;

# if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
# endif

/** check if specified string is actually an ip address */
unsigned int is_ipaddr(const char *string);

/** check if IPv4 address belongs to private network */
unsigned int is_ipv4_local(const char *ip_addr);

/** check if addr belongs to specified network */
unsigned compare_netmask(const char *addr, const char *net, unsigned prefix_len);

#ifdef WITH_IPV6
/** check if IPv6 address belongs to private network */
unsigned int is_ipv6_local(const char *ip_addr);
#endif

/** describe rfs option (do NOT free() result) */
const char* describe_option(const enum rfs_export_opts option);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* UTILS_H */
