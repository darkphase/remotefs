
#include <errno.h>

#include "exception.hpp"
#include "server.hpp"
#include "sockets.hpp"

#include "../../src/server.h"
#include "../../src/exports.h"
#include "../../src/sendrecv_server.h"

Stub::Server::Server()
: m_listen_socket(-1)
, m_accepted_socket(-1)
{
	memset(&m_instance, 0, sizeof(m_instance));
	init_rfsd_instance(&m_instance);

	m_instance.server.directory_mounted = 1;
	m_instance.server.mounted_export = new rfs_export;
	m_instance.server.mounted_export->path = strdup("/");
#ifdef WITH_UGO
	m_instance.server.mounted_export->options = OPT_UGO;
#endif

	m_listen_socket = create_socket(socket_name());
	if (m_listen_socket < 0)
	{
		throw EXCEPTION(-m_listen_socket);
	}
}

Stub::Server::~Server()
{
	if (m_instance.server.mounted_export != 0)
	{
		free(m_instance.server.mounted_export->path);
		delete m_instance.server.mounted_export;
	}

	if (m_listen_socket != -1)
	{
		close_socket(m_listen_socket, socket_name());
	}

	if (m_accepted_socket != -1)
	{
		close(m_accepted_socket);
	}
}

int Stub::Server::accept()
{
	m_accepted_socket = accept_socket(m_listen_socket);
	if (m_accepted_socket < 0)
	{
		throw EXCEPTION(-m_accepted_socket);
	}

	m_instance.sendrecv.socket = m_accepted_socket;

	return m_accepted_socket;
}

void Stub::Server::recv_command(struct command &cmd)
{
	if (rfs_receive_cmd(&m_instance.sendrecv, &cmd) < 0)
	{
		throw EXCEPTION(ECONNABORTED); 
	}
}

void Stub::Server::send_answer(struct answer &ans)
{
	if (rfs_send_answer(&m_instance.sendrecv, &ans) < 0)
	{
	    throw EXCEPTION(ECONNABORTED);
	}
}

