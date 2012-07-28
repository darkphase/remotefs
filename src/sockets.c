/*
remotefs file system
See the file AUTHORS for copyright information.

This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#if defined QNX || defined FREEBSD || defined DARWIN
#include <sys/time.h>
#endif
#if defined FREEBSD
#include <netinet/in.h>
#else
#include <netinet/ip.h>
#endif
#include <netinet/tcp.h>

#include "config.h"

int setup_socket_reuse(int socket, const char reuse)
{
	int reuse_copy = reuse;
	errno = 0;
	return setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &reuse_copy, sizeof(reuse_copy)) == 0 ? 0 : -errno;
}

int setup_socket_ndelay(int socket, const char nodelay)
{
	int arg = nodelay;
	errno = 0;
	return setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &arg, sizeof(arg)) == 0 ? 0 : -errno;
}

int setup_soket_pid(int socket, const pid_t pid)
{
	errno = 0;
	return fcntl(socket, F_SETOWN, pid) == pid ? 0 : -errno;
}

int setup_socket_recv_timeout(int socket, const unsigned usecs)
{
	errno = 0;
	const struct timeval timeout = { usecs / 1000000, usecs % 1000000 };
	return setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == 0 ? 0 : -errno;
}

int setup_socket_send_timeout(int socket, const unsigned usecs)
{
	errno = 0;
	const struct timeval timeout = { usecs / 1000000, usecs % 1000000 };
	return setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) == 0 ? 0 : -errno;
}

int setup_socket_non_blocking(int socket)
{
	long opt = fcntl(socket, F_GETFL, NULL);
	if (opt < 0)
	{
		return opt;
	}

	opt |= O_NONBLOCK;

	opt = fcntl(socket, F_SETFL, opt);
	return (opt < 0 ? opt : 0);
}

int setup_socket_blocking(int socket)
{
	long opt = fcntl(socket, F_GETFL, NULL);
	if (opt < 0)
	{
		return opt;
	}

	opt ^= O_NONBLOCK;

	opt = fcntl(socket, F_SETFL, opt);
	return (opt < 0 ? opt : 0);
}

#ifdef WITH_IPV6
int setup_socket_ipv6_only(int socket)
{
	int on = 1;

	errno = 0;
	return setsockopt(socket, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on))  == 0 ? 0 : -errno;
}
#endif /* WITH_IPV6 */
