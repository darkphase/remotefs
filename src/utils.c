/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#if defined QNX || defined FREEBSD
# include <sys/socket.h>
#endif
#if defined FREEBSD
# include <netinet/in.h>
#endif

#include "config.h"
#include "list.h"
#include "sendrecv.h"
#include "utils.h"

unsigned int is_ipaddr(const char *string)
{
#ifdef WITH_IPV6
	if (strchr(string, ':') != NULL)
	{
		/* may be an IPv6 address */
		struct sockaddr_in6 addr = { 0 };
		return inet_pton(AF_INET6, string, &(addr.sin6_addr)) == 1 ? 1 : 0;
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

static unsigned compare_maskbits(const unsigned char *addr, const unsigned char *net, unsigned bits_number)
{
	unsigned bits_checked = 0;
	unsigned byte = 0; for (; byte < (bits_number + 7) / 8 && bits_checked < bits_number; ++byte)
	{
		unsigned bit = 0; for (; bit < 8 && bits_checked < bits_number; ++bit, ++bits_checked)
		{
			uint8_t mask = 1 << (7 - bit); /* bit value which must be the same */
			if ( (addr[byte] & mask) != (net[byte] & mask) )
			{
				/* not from the same subnet */
				return 0;
			}
		}
	}

	return 1;
}

#ifdef WITH_IPV6
static unsigned compare_ipv6_netmask(const char *addr, const char *net, unsigned prefix_len)
{
	DEBUG("comparing %s with %s/%u assuming it's ipv6\n", addr, net, prefix_len);

	if ( prefix_len > 128 )
	{
		return 0;
	}

	struct in6_addr addr6;
	if (inet_pton(AF_INET6, addr, &addr6) != 1)
	{
		return 0;
	}

	struct in6_addr mask6;
	if (inet_pton(AF_INET6, net, &mask6) != 1)
	{
		return 0;
	}

	return compare_maskbits(addr6.s6_addr, mask6.s6_addr, prefix_len);
}
#endif

static unsigned compare_ipv4_netmask(const char *addr, const char *net, unsigned prefix_len)
{
	DEBUG("comparing %s with %s/%u assuming it's ipv4\n", addr, net, prefix_len);

	if ( prefix_len > 32 )
	{
		return 0;
	}

	struct in_addr addr4;
	if (inet_pton(AF_INET, addr, &addr4) != 1)
	{
		return 0;
	}
	
	struct in_addr net4;
	if (inet_pton(AF_INET, net, &net4) != 1)
	{
		return 0;
	}

	return compare_maskbits((const unsigned char *)&addr4.s_addr, (const unsigned char *)&net4.s_addr, prefix_len);
}

unsigned compare_netmask(const char *addr, const char *net, unsigned prefix_len)
{
#ifdef WITH_IPV6
	unsigned is_ipv6_addr = (strchr(addr, ':') != NULL ? 1 : 0);
	unsigned is_ipv6_net  = (strchr(net, ':') != NULL ? 1 : 0);

	if (is_ipv6_addr != 0 && is_ipv6_net != 0)
	{
		return compare_ipv6_netmask(addr, net, prefix_len);
	}
	else if (is_ipv6_addr != is_ipv6_net)
	{
		return 0;
	}
#endif
	
	return compare_ipv4_netmask(addr, net, prefix_len);
}

#ifdef WITH_IPV6
unsigned int is_ipv6_local(const char *ip_addr)
{
	if (strstr(ip_addr, "fe") != ip_addr
	&& strncmp(ip_addr, "::1", sizeof("::1")) != 0)
	{
		if (strstr(ip_addr, "ffff::") == ip_addr 
		&& strchr(ip_addr, '.') > ip_addr + 6)
		{
			return is_ipv4_local(ip_addr + 6);
		}

		return 0;
	}
	
	return 1;
}
#endif /* WITH_IPV6 */

#ifdef WITH_EXPORTS_LIST
const char* describe_option(const enum rfs_export_opts option)
{
	switch (option)
	{
	case OPT_NONE:              return "None";
	case OPT_RO:                return "Read-only";
#ifdef WITH_UGO
	case OPT_UGO:               return "UGO-compatible";
#endif
	case OPT_COMPAT:            break;
	}
	
	return "Unknown";
}
#endif /* WITH_EXPORTS_LIST */
