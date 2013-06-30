
#include <errno.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "sockets.hpp"

const char* Stub::socket_name()
{
	return "/tmp/debug_socket";
}

int Stub::close_socket(int sock, const char *socket_name)
{
	close(sock);
	unlink(socket_name);
	shutdown(sock, SHUT_RDWR);
	return 0;
}

int Stub::create_socket(const char *socket_name)
{
	int sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock < 0)
	{
		return -errno;
	}

	struct sockaddr_un addr = { 0 };
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, socket_name);

	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		int saved_errno = errno;
		close_socket(sock, socket_name);
		return -saved_errno;
	}

	if (listen(sock, 1) < 0)
	{
		int saved_errno = errno;
		close_socket(sock, socket_name);
		return -saved_errno;
	}

	return sock;
}

int Stub::accept_socket(int sock)
{
	int accepted = accept(sock, NULL, NULL);
	if (accepted < 0)
	{
		return -errno;
	}

	return accepted;
}

int Stub::connect_socket(const char *socket_name)
{
	int sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock < 0)
	{
		return -errno;
	}
	
	struct sockaddr_un addr = { 0 };
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, socket_name);

	if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		int saved_errno = errno;
		close(sock);
		return -saved_errno;
	}

	return sock;
}

