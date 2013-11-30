
#include <sys/socket.h>
#include <cppunit/extensions/HelperMacros.h>

#include "op_conn_aborted.hpp"

namespace
{
	int handler_conn_aborted(Stub::Server &server, const rfs_command &cmd)
	{
		shutdown(server.get_accepted_socket(), SHUT_RDWR);
		return 0;
	}
}

void TestOpConnAborted::testOpConnAborted()
{
	try
	{
		Stub::Framework::ret_t ret = Stub::Framework::get_instance()->communicate(handler_conn_aborted, m_caller);
		CPPUNIT_ASSERT(ret.caller == ECONNABORTED);
	}
	catch (const Stub::Exception &e)
	{
		CPPUNIT_ASSERT_MESSAGE(e.describe(), false);
	}
}

