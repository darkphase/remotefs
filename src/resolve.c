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
#include "resolve.h"

struct rfs_list* host_ips(const char *host, int *address_family)
{
	struct addrinfo *addr_info = NULL;
	struct addrinfo hints = { 0 };

	/* resolve name or address */
#ifdef WITH_IPV6
	hints.ai_family    = AF_UNSPEC;
#else
	hints.ai_family    = AF_INET;
#endif
	hints.ai_socktype  = SOCK_STREAM;

	if (address_family != NULL)
	{
		hints.ai_family = *address_family;
	}

	DEBUG("looking for family: %s\n", (hints.ai_family == AF_INET6 ? "v6" : "v4 (or unspec)"));

	int resolve_ret = getaddrinfo(host, NULL, &hints, &addr_info);
	if (resolve_ret != 0)
	{
		return NULL;
	}

	struct rfs_list *addresses = NULL;

	struct addrinfo *current = addr_info;
	while (current != NULL)
	{
		const unsigned max_len = 256;
		char *ip = malloc(max_len);

		if (inet_ntop(current->ai_family,
#ifdef WITH_IPV6
		current->ai_family == AF_INET6 ?
		(const void *)&((struct sockaddr_in6 *)current->ai_addr)->sin6_addr :
#endif
		(const void *)&((struct sockaddr_in *)current->ai_addr)->sin_addr,
		ip,
		max_len - 1) == NULL)
		{
			free(ip);
			destroy_list(&addresses);

			return NULL;
		}

		/* filter duplicates */
		int filtered = 0;

		struct rfs_list *addr_rec = addresses;
		while (addr_rec != NULL)
		{
			struct resolved_addr *rec = (struct resolved_addr *)addr_rec->data;
			if (strcmp(rec->ip, ip) == 0)
			{
				filtered = 1;
				break;
			}

			addr_rec = addr_rec->next;
		}

		if (filtered != 0)
		{
			free(ip);
			current = current->ai_next;

			continue;
		}

		struct resolved_addr *rec = malloc(sizeof(*rec));
		rec->ip = ip;
		rec->addr_family = current->ai_family;

		add_to_list(&addresses, rec);

		DEBUG("found host: %s\n", ip);

		current = current->ai_next;
	}

	freeaddrinfo(addr_info);

	return addresses;
}
