
#include <unistd.h>

#include "client.hpp"
#include "exception.hpp"
#include "sockets.hpp"

Stub::Client::Client()
: m_socket(-1)
{
	memset(&m_instance, 0, sizeof(m_instance));
	init_rfs_instance(&m_instance);
}

Stub::Client::~Client()
{
	if (m_socket != -1)
	{
		close(m_socket);
	}
}

int Stub::Client::connect()
{
	m_socket = connect_socket(socket_name());
	
	if (m_socket < 0)
	{
		throw EXCEPTION(-m_socket);
	}

	m_instance.sendrecv.socket = m_socket;

	return m_socket;
}

