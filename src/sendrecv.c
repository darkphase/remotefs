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
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#if defined SOLARIS
#include <sys/sockio.h>
#endif
#include <unistd.h>

#include "buffer.h"
#include "command.h"
#include "config.h"
#include "error.h"
#include "instance.h"
#include "sendrecv.h"
#ifdef WITH_SSL
#include "ssl.h"
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

	while (next != NULL)
	{
		errno = 0;

		if (next->ai_family == AF_INET)
		{
			struct sockaddr_in *addr = (struct sockaddr_in *)next->ai_addr;
			addr->sin_port = htons(port);
			sock = socket(AF_INET, SOCK_STREAM, 0);
		}
#ifdef WITH_IPV6
		else
		{
			struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)next->ai_addr;
			addr6->sin6_port = htons(port);
			sock = socket(AF_INET6, SOCK_STREAM, 0);
		}
#endif
		if (sock == -1)
		{
			printf("sock = %d, errno %s\n",sock, strerror(errno));
			next = next->ai_next;
		}
		else
		{
			errno = 0;
			if (connect(sock, (struct sockaddr *)next->ai_addr, next->ai_addrlen) == -1)
			{
				close(sock);
				sock = -1;
			}
			else
			{
				break;
			}
		}

		next = next->ai_next;
	}

	freeaddrinfo(addr_info);

	if ( sock == -1 )
	{
		return -errno;
	}

	info->socket = sock;
	info->connection_lost = 0;
	
	return sock;
}

#ifdef RFS_DEBUG
static void dump_iov(struct iovec *iov, unsigned count)
{
	DEBUG("dumping %u io vectors:\n", count);
	unsigned i = 0; for (i = 0; i < count; ++i)
	{
		DEBUG("%u: base: %p, len: %u\n", i, iov[i].iov_base, (unsigned int)iov[i].iov_len);
	}
}
#endif

static int fix_iov(struct iovec *iov, unsigned count, size_t size_left)
{
	size_t overall_size = 0;
	
	{
	int i = 0; for (i = 0; i < count; ++i)
	{
		overall_size += iov[i].iov_len;
	}
	}
	
	if (size_left == overall_size)
	{
		return count;
	}
	
	size_t diff = overall_size - size_left;
	
	int ret = count;
	while (1)
	{
#ifdef RFS_DEBUG
		DEBUG("count: %u, size left: %u, overall_size: %u, diff: %u\n", count, (unsigned int)size_left, (unsigned int)overall_size, (unsigned int)diff);
		dump_iov(iov, ret);
#endif
		if (ret < 1)
		{
			DEBUG("%s\n", "error while fixing iovs");
			break; /* actually error */
		}
		
		if (iov[0].iov_len > diff)
		{
			iov[0].iov_base = (char*)iov[0].iov_base + diff;
			iov[0].iov_len -= diff;
			break;
		}
		else if (diff != 0)
		{
			diff -= iov[0].iov_len;
			
			int j = 0; for (j = 0; j < ret; ++j)
			{
				iov[j].iov_len = iov[j + 1].iov_len;
				iov[j].iov_base = iov[j + 1].iov_base;
			}
			
			--ret;
			
			continue;
		}
	}
	
#ifdef RFS_DEBUG
	dump_iov(iov, ret);
#endif
	
	return ret;
}

ssize_t rfs_writev(struct sendrecv_info *info, struct iovec *iov, unsigned count)
{
	size_t overall_size = 0;
	int i = 0; for (i = 0; i < count; ++i)
	{
		overall_size += iov[i].iov_len;
	}
	
	ssize_t size_sent = 0;
	while (size_sent < overall_size)
	{
		count = fix_iov(iov, count, overall_size - size_sent);
		
		if (count == 0)
		{
			return -EIO;
		}
		
		int done = 0;
		
		errno = 0;
#ifdef WITH_SSL
		if (info->ssl_enabled != 0)
		{
			done = rfs_ssl_write(info->ssl_socket, iov[0].iov_base, iov[0].iov_len);
		}
		else
		{
#endif
			done = writev(info->socket, iov, count);
#ifdef WITH_SSL
		}
#endif
		if (done <= 0)
		{
			if (errno == EAGAIN || errno == EINTR)
			{
				continue;
			}
			
			if (errno != 0)
			{
				DEBUG("%s encountered\n", strerror(errno));
			}
			
			DEBUG("connection lost in rfs_writev, size_sent: %u, %s\n", (unsigned int)size_sent, strerror(errno));
			info->connection_lost = 1;
			/* see comment for rfs_recv */
			return errno == 0 ? -ECONNABORTED : -errno;
		}
		
		size_sent += done;
	}
	
#ifdef RFS_DEBUG
	if (size_sent > 0)
	{
		info->bytes_sent += size_sent;
	}
#endif
	
	return size_sent;
}

static inline int is_mark(int socket)
{
	/* wait for further data become ready to not miss OOB byte */
	DEBUG("%s\n", "waiting for further data");

	fd_set read_set;
	FD_SET(socket, &read_set);

	int select_ret = 0;
	do
	{
		select_ret = select(socket + 1, &read_set, NULL, NULL, NULL);

		if (select_ret < 0)
		{
			return -errno;
		}
	}
	while (select_ret < 1);

	int atmark = 0;

	if (ioctl(socket, SIOCATMARK, &atmark) < 0)
	{
		return -EIO;
	}

	DEBUG("atmark: %d\n", atmark);

	if (atmark != 0)
	{
		char oob = 0;
		return (recv(socket, (char *)&oob, 1, MSG_OOB) < 0 ? -1 : 1);
	}

	return 0;
}

ssize_t rfs_recv(struct sendrecv_info *info, char *buffer, size_t size, unsigned check_oob)
{
	ssize_t size_recv = 0;
	while (size_recv < size)
	{
		errno = 0;
		ssize_t done = 0;
#ifdef WITH_SSL
		if (info->ssl_enabled != 0)
		{
			DEBUG("%s\n", "using SSL for read");
			done = rfs_ssl_read(info->ssl_socket, buffer + size_recv, size - size_recv);
		}
		else
		{
#endif
			if (check_oob != 0)
			{
				/* is_mark() will clear stream from OOB message */
				int check_mark = is_mark(info->socket);

				DEBUG("is mark: %d\n", check_mark);

				if (check_mark < 0)
				{
					return check_mark;
				}
				else if (check_mark != 0)
				{
					info->oob_received = 1;
					return -EIO;
				}
			}
			
			done = recv(info->socket, buffer + size_recv, size - size_recv, MSG_WAITALL);
#ifdef WITH_SSL
		}
#endif

#ifdef RFS_DEBUG
		if (done != size - size_recv)
		{
			DEBUG("recv call has been interrupted with %s (%d), recv result is %ld\n", strerror(errno), errno, (long int)done);
		}
#endif

		if (done <= 0)
		{
			if (errno == EAGAIN || errno == EINTR)
			{
				continue;
			}
			
			if (errno != 0) /* THE CAKE IS A LIE */
			{
				DEBUG("%s encountered\n", strerror(errno));
			}
			
			DEBUG("connection lost in rfs_recv, size_recv: %d, %s\n", (int)size_recv, strerror(errno));
			info->connection_lost = 1;
			/* if connection lost, then we could get Success
			so fake ret code for upper layer to be able to recognize error */
			return errno == 0 ? -ECONNABORTED : -errno;
		}
		
		size_recv += (size_t)done;
	}

#ifdef RFS_DEBUG
	info->bytes_recv += size_recv;
#endif
	
	return size_recv;
}

ssize_t rfs_ignore_incoming_data(struct sendrecv_info *info, const size_t data_len)
{
	size_t size_ignored = 0;
	char buffer[4096] = { 0 };
	
	while (size_ignored < data_len)
	{
		ssize_t ret = rfs_recv(info, 
			buffer, 
			(data_len - size_ignored > sizeof(buffer) ? sizeof(buffer) : data_len - size_ignored), 
			0);

		if (ret < 0)
		{
			return ret;
		}
		
		size_ignored += (size_t)ret;
	}
	
	return size_ignored;
}

#ifdef RFS_DEBUG
void dump_sendrecv_stats(struct sendrecv_info *info)
{
	DEBUG("%s\n", "dumping transfer statistics");
	DEBUG("bytes sent: %lu (%.2fM, %.2fK)\n", info->bytes_sent, (float)info->bytes_sent / (1024 * 1024), (float)info->bytes_sent / 1024);
	DEBUG("bytes recv: %lu (%.2fM, %.2fK)\n", info->bytes_recv, (float)info->bytes_recv / (1024 * 1024), (float)info->bytes_recv / 1024);
}
#endif

