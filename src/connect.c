/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#if defined SOLARIS
#include <sys/sockio.h>
#endif
#include <unistd.h>

#include "config.h"
#include "instance.h"
#ifdef WITH_SSL
#include "ssl/ssl.h"
#endif

#include <netinet/tcp.h>

int rfs_connect(struct sendrecv_info *info, const char *host, unsigned port, unsigned force_ipv4, unsigned force_ipv6)
{
	struct addrinfo *addr_info = NULL;
	struct addrinfo hints = { 0 };

	/* resolve name or address */
#if defined WITH_IPV6
	hints.ai_family    = AF_UNSPEC;
#else
	hints.ai_family    = AF_INET;
#endif
	hints.ai_socktype  = SOCK_STREAM; 
	hints.ai_flags     = AI_ADDRCONFIG;
#if defined WITH_IPV6
	if ( force_ipv4 )
	{
		hints.ai_family = AF_INET;
	}
	else if ( force_ipv6 )
	{
		hints.ai_family = AF_INET6;
	}
#endif

	int resolve_result = getaddrinfo(host, NULL, &hints, &addr_info);
	if (resolve_result != 0)
	{
		DEBUG("Can't resolve address for %s : %s\n", host, gai_strerror(resolve_result));
		return -EHOSTUNREACH;
	}
	
	int sock = -1;
	struct addrinfo *next = addr_info;
	int saved_errno = 0;

	while (next != NULL)
	{
		sock = -1;

		if (next->ai_family == AF_INET)
		{
			struct sockaddr_in *addr = (struct sockaddr_in *)next->ai_addr;
			addr->sin_port = htons(port);
			sock = socket(AF_INET, SOCK_STREAM, 0);
		}
#ifdef WITH_IPV6
		else if (next->ai_family == AF_INET6)
		{
			struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)next->ai_addr;
			addr6->sin6_port = htons(port);
			sock = socket(AF_INET6, SOCK_STREAM, 0);
		}
#endif
		if (sock == -1)
		{
			DEBUG("sock = %d, errno %s\n",sock, strerror(errno));
			
			next = next->ai_next;
			continue;
		}
			
		errno = 0;
		if (connect(sock, (struct sockaddr *)next->ai_addr, next->ai_addrlen) == -1)
		{
			saved_errno = errno;
			close(sock);
			sock = -1;
		}
		else
		{
			break;
		}

		next = next->ai_next;
	}

	freeaddrinfo(addr_info);

	if (sock > 0)
	{
		info->socket = sock;
		info->connection_lost = 0;
	}
	
	return (sock <= 0 ? -saved_errno : sock);
}
