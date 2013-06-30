#ifndef TEST_OP_CONN_ABORTED_HPP
#define TEST_OP_CONN_ABORTED_HPP

#include "base_op_test.hpp"

class TestOpConnAborted : public BaseOpTest
{
public:
	TestOpConnAborted(Stub::Framework::caller_t caller) : BaseOpTest(caller) {}
	void testOpConnAborted();
};

#endif // TEST_OP_CONN_ABORTED_HPP

