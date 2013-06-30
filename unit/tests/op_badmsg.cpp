
#include <cppunit/extensions/HelperMacros.h>

#include "op_badmsg.hpp"

namespace
{
	int handler_badmsg(Stub::Server &server, const command &cmd)
	{
		answer ans = { cmd.command + 1 < cmd_last ? cmd.command + 1 : cmd.command - 1, 0, 0, 0 };
		server.send_answer(ans);
		return 0;
	}
}

void TestOpBadMsg::testOpBadMsg()
{
	try
	{
		Stub::Framework::ret_t ret = Stub::Framework::get_instance()->communicate(handler_badmsg, m_caller);
		CPPUNIT_ASSERT(ret.caller == EBADMSG);
	}
	catch (const Stub::Exception &e)
	{
		CPPUNIT_ASSERT_MESSAGE(e.describe(), false);
	}
}

