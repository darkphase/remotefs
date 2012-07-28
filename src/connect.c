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
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#if defined SOLARIS
#include <sys/sockio.h>
#endif
#include <unistd.h>

#include "config.h"
#include "instance.h"
#include "list.h"
#include "resolve.h"
#include "sockets.h"

#include <netinet/tcp.h>

int rfs_connect(struct sendrecv_info *info, const char *host, unsigned port, unsigned force_ipv4, unsigned force_ipv6)
{
	struct list *ips = host_ips(host, NULL);

	if (ips == NULL)
	{
		DEBUG("Can't resolve address for %s\n", host);
		return -EHOSTUNREACH;
	}

	int sock = -1;
	int saved_errno = 0;

	union
	{
		struct sockaddr_storage ss;
		struct sockaddr_in si;
		struct sockaddr_in6 si6;
	} sockaddr;

	struct list *resolved_item = ips;

	while (resolved_item != NULL)
	{
		struct resolved_addr *addr = (struct resolved_addr *)resolved_item->data;

		sock = -1;

		int pton_result = 0;
		sockaddr.ss.ss_family = addr->addr_family;
		switch (addr->addr_family)
		{
		case AF_INET:
			{
			pton_result = inet_pton(addr->addr_family, addr->ip, &(sockaddr.si.sin_addr));
			sockaddr.si.sin_port = htons(port);
			}
			break;
#ifdef WITH_IPV6
		case AF_INET6:
			{
			pton_result = inet_pton(addr->addr_family, addr->ip, &(sockaddr.si6.sin6_addr));
			sockaddr.si6.sin6_port = htons(port);
			}
			break;
#endif
		default:
			break;
		}

		if (pton_result != 1)
		{
			saved_errno = EINVAL;
			resolved_item = resolved_item->next;
			continue;
		}

		sock = socket(sockaddr.ss.ss_family, SOCK_STREAM, 0);

		if (sock == -1)
		{
			DEBUG("sock = %d, errno %s\n", sock, strerror(errno));

			resolved_item = resolved_item->next;
			continue;
		}

		DEBUG("%s\n", "connecting...");

		setup_socket_non_blocking(sock);

		errno = 0;
		int connect_ret = connect(sock, (struct sockaddr *)&sockaddr.ss, sizeof(sockaddr.ss));
		if (connect_ret != 0)
		{
			saved_errno = errno;

			if (errno == EINPROGRESS)
			{
				fd_set fds;
				FD_ZERO(&fds);
				FD_SET(sock, &fds);

				struct timeval timeout = { DEFAULT_CONNECT_TIMEOUT / 1000000,
					DEFAULT_CONNECT_TIMEOUT % 1000000 };

				DEBUG("%s\n", "selecting...");

				int select_ret = select(sock + 1, NULL, &fds, NULL, &timeout);

				connect_ret = (select_ret > 0 ? 0 : connect_ret);
				saved_errno = (select_ret == 0 ? ETIMEDOUT : errno);
			}
		}

		setup_socket_blocking(sock);

		if (connect_ret != 0)
		{
			close(sock);
			sock = -1;
		}

		resolved_item = resolved_item->next;
	}

	destroy_list(&ips);

	if (sock > 0)
	{
		DEBUG("%s\n", "connected");

		info->socket = sock;
		info->connection_lost = 0;
	}

	return (sock <= 0 ? -saved_errno : sock);
}
