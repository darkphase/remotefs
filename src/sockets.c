/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#if defined QNX
#include <sys/time.h>
#endif

#include "sockets.h"

int setup_socket_timeout(int socket, const int timeout)
{
	struct timeval socket_timeout = { timeout, 0 };
	errno = 0;
	if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &socket_timeout, sizeof(socket_timeout)) != 0)
	{
		return -errno;
	}

	errno = 0;
	if (setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &socket_timeout, sizeof(socket_timeout)) != 0)
	{
		return -errno;
	}
	
	return 0;
}

int setup_socket_buffer(int socket, const int size)
{
	errno = 0;
	if (setsockopt(socket, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)) != 0)
	{
		return -errno;
	}

	if (setsockopt(socket, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)) != 0)
	{
		return -errno;
	}
	return 0;
}

int setup_socket_reuse(int socket, const char reuse)
{
	int reuse_copy = reuse;
	errno = 0;
	return setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &reuse_copy, sizeof(reuse_copy)) == 0 ? 0 : -errno;
}

int setup_soket_pid(int socket, const pid_t pid)
{
	errno = 0;
	return fcntl(socket, F_SETOWN, pid) == pid ? 0 : -errno;
}

