/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#if defined QNX || defined FREEBSD
# include <sys/socket.h>
#endif
#include <netdb.h>
#include <stdlib.h>

#include "config.h"
#include "utils.h"
#include "sendrecv.h"

unsigned int is_ipaddr(const char *string)
{
#ifdef WITH_IPV6
	if (strchr(string, ':'))
	{
		/* may be an IPv6 address */
		struct sockaddr_in6 addr = { 0 };
		return inet_pton(AF_INET6, string, &(addr.sin6_addr)) == -1 ? 0 : 1;
	}
	else
	{
#endif
		return inet_addr(string) == INADDR_NONE ? 0 : 1;
#ifdef WITH_IPV6
	}
#endif
}

unsigned int is_ipv4_local(const char *ip_addr)
{
	if (strstr(ip_addr, "192.168.") != ip_addr
	&& strstr(ip_addr, "127.0.0.") != ip_addr
	&& strstr(ip_addr, "10.") != ip_addr
	&& strstr(ip_addr, "172.16.") != ip_addr
	&& strstr(ip_addr, "1.") != ip_addr
	&& strstr(ip_addr, "2.") != ip_addr)
	{
		return 0;
	}
	
	return 1;
}

#ifdef WITH_IPV6
unsigned int is_ipv6_local(const char *ip_addr)
{
	if (strstr(ip_addr, "fe") != ip_addr
	&& strncmp(ip_addr, "::1", sizeof("::1")) != 0)
	{
		return 0;
	}
	
	return 1;
}
#endif /* WITH_IPV6 */

char* host_ip(const char *host, int *resolved_address_family)
{
	char *result = NULL;
	*resolved_address_family = AF_UNSPEC;
	
	struct addrinfo *addr_info = NULL;
	struct addrinfo hints = { 0 };

	/* resolve name or address */
	hints.ai_family    = AF_UNSPEC;
	hints.ai_socktype  = SOCK_STREAM; 
	hints.ai_flags     = AI_ADDRCONFIG;
	
	int resolve_ret = getaddrinfo(host, NULL, &hints, &addr_info);
	if (resolve_ret == 0)
	{
		*resolved_address_family = addr_info->ai_family;
		
		unsigned max_len = 255;
		result = malloc(max_len);
		
		inet_ntop(addr_info->ai_family, 
		&((struct sockaddr_in *)addr_info->ai_addr)->sin_addr,
		result,
		max_len - 1);
		
		freeaddrinfo(addr_info);
		
		DEBUG("real host: %s\n", result);
	}
	
	return result;
}

#ifdef WITH_EXPORTS_LIST
const char* describe_option(const enum rfs_export_opts option)
{
	switch (option)
	{
	case opt_none:              return "None";
	case opt_ro:                return "Read-only";
	case opt_ugo:               return "UGO-compatible";
	case opt_compat:            break;
	}
	
	return "Unknown";
}
#endif /* WITH_EXPORTS_LIST */
