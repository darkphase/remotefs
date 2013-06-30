#ifndef TEST_OP_BADMSG_HPP
#define TEST_OP_BADMSG_HPP

#include "base_op_test.hpp"

class TestOpBadMsg : public BaseOpTest
{
public:
	TestOpBadMsg(Stub::Framework::caller_t caller) : BaseOpTest(caller) {}
	void testOpBadMsg();
};

#endif // TEST_OP_BADMSG_HPP

