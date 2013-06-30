
#include "../src/operations/operations.h"
#include "framework_test.hpp"
#include "op_conn_aborted.hpp"
#include "op_badmsg.hpp"

class TestOpOpen 
	: public FrameworkTest
	, public TestOpConnAborted
	, public TestOpBadMsg
{
	CPPUNIT_TEST_SUITE(TestOpOpen);
	CPPUNIT_TEST(testOpConnAborted);
	CPPUNIT_TEST(testOpBadMsg);
	CPPUNIT_TEST_SUITE_END();

public:
	TestOpOpen();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestOpOpen);

namespace 
{
	int call_valid_open(Stub::Client &client)
	{
		uint64_t desc = 0;
		return _rfs_open(&client.get_instance(), "./" __FILE__, 0, &desc);
	}
}

TestOpOpen::TestOpOpen()
: TestOpConnAborted(call_valid_open)
, TestOpBadMsg(call_valid_open)
{
}

