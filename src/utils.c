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
#ifndef WITH_IPV6
	return inet_addr(string) == INADDR_NONE ? 0 : 1;
#else
	if (strchr(string,':'))
	{
		/* may be an IPv6 address */
		struct sockaddr_in6 addr;
		return inet_pton(AF_INET6, string, &(addr.sin6_addr)) == -1 ? 0 : 1;
	}
	else
	{
		return inet_addr(string) == INADDR_NONE ? 0 : 1;
	}
#endif
}

unsigned int is_ipv4_local(const char *ip_addr)
{
	if (strstr(ip_addr, "192.168.") != ip_addr
	&& strstr(ip_addr, "127.0.0.") != ip_addr
	&& strstr(ip_addr, "10.") != ip_addr
	&& strstr(ip_addr, "172.16.") != ip_addr)
	{
		return 0;
	}
	
	return 1;
}

char* resolve_ipv4(const char *host)
{
	char *result = NULL;

	struct hostent *host_ent = gethostbyname(host);
	if (host_ent != NULL)
	{
		
		unsigned max_len = 255;
		result = malloc(max_len);
		
		inet_ntop(host_ent->h_addrtype, 
		host_ent->h_addr_list[0],
		result,
		max_len - 1);
		
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
#endif

#ifdef WITH_IPV6
unsigned int is_ipv6_local(const char *ip_addr)
{
	return 1;
}

char* resolve_ipv6(const char *host)
{
	return NULL;
}
#endif
