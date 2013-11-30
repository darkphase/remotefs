
#ifndef STUB_FRAMEWORK_HPP
#define STUB_FRAMEWORK_HPP

#include <errno.h>
#include <stdlib.h>
#include <sys/wait.h>

#include <memory>

#include "client.hpp"
#include "exception.hpp"
#include "server.hpp"
#include "sockets.hpp"

namespace Stub
{
	class Framework
	{
	public:
		typedef int (*caller_t) (Client &client);
		typedef int (*handler_t) (Server &server, const rfs_command &cmd);

	public:
		struct ret_t
		{
			int handler;
			int caller;

			ret_t(int hret, int cret) : handler(hret), caller(cret) {}
		};

	public:
		virtual ~Framework() {};

		static Framework* get_instance();
		static void reset();

		ret_t communicate(handler_t handler, caller_t caller);

	private:
		Framework();

	protected:
		Server m_server;
		Client m_client;

	private:
		static std::auto_ptr<Framework> m_instance;
	};
}

#endif // STUB_FRAMEWORK_HPP

