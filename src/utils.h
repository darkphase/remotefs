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

struct list;

struct resolved_addr
{
	char *ip;
	int addr_family;
};

/** check if specified string is actually an ip address */
unsigned int is_ipaddr(const char *string);

/** check if IPv4 address belongs to private network */
unsigned int is_ipv4_local(const char *ip_addr);

/** don't forget to free() returned value */
char* host_ip(const char *host, int *resolved_address_family);

/** don't forget to free() returned value */
struct list* host_ips(const char *host, int *address_family);

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
