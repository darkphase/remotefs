#ifndef STUB_SOCKETS_HPP
#define STUB_SOCKETS_HPP

namespace Stub
{
	const char* socket_name();
	int close_socket(int sock, const char *socket_name);
	int create_socket(const char *socket_name);
	int accept_socket(int sock);
	int connect_socket(const char *socket_name);
}

#endif // STUB_SOCKETS_HPP

