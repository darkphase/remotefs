#ifndef STUB_CLIENT_HPP
#define STUB_CLIENT_HPP

#include "../../src/instance_client.h"

namespace Stub
{
	class Client
	{
	public:
		Client();
		virtual ~Client();
		virtual int connect();
		virtual int get_socket() const { return m_socket; }

		rfs_instance& get_instance() { return m_instance; }

	protected:
		int m_socket;
		rfs_instance m_instance;
	};
}

#endif // STUB_CLIENT_HPP

