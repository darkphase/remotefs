/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef UTILS_H
#define UTILS_H

/** various utils */

/** check if specified user is actually an ip address */
unsigned int is_ipaddr(const char *string);

/** is IPv4 address belongs to private network */
unsigned int is_ipv4_local(const char *ip_addr);

/** don't forget to free() returned value */
char* host_ip(const char *host, int *resolved_address_family);

#ifdef WITH_IPV6
/** is IPv6 address belongs to private network */
unsigned int is_ipv6_local(const char *ip_addr);

#endif

/** describe rfs option (do NOT free() result) */
const char* describe_option(const enum rfs_export_opts option);

#endif /* UTILS_H */
