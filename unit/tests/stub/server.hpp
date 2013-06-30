#ifndef STUB_SERVER_HPP
#define STUB_SERVER_HPP

#include "../../src/command.h"
#include "../../src/instance_server.h"

namespace Stub
{
	class Server
	{
	public:
		Server();
		virtual ~Server();

		virtual int accept();
		virtual int get_listen_socket() const { return m_listen_socket; }
		virtual int get_accepted_socket() const { return m_accepted_socket; }
		
		virtual void recv_command(command &cmd);
		virtual void send_answer(answer &ans);
	
		rfsd_instance& get_instance() { return m_instance; }

	protected:
		int m_listen_socket;
		int m_accepted_socket;

		rfsd_instance m_instance;
	};
}

#endif // STUB_SERVER_HPP

