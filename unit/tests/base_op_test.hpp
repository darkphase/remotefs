#ifndef BASE_OP_TEST_HPP
#define BASE_OP_TEST_HPP

#include "stub/framework.hpp"

class BaseOpTest
{
public:
	BaseOpTest(Stub::Framework::caller_t caller) : m_caller(caller) {}
	virtual ~BaseOpTest() {}

protected:
	Stub::Framework::caller_t m_caller;
};

#endif // BASE_OP_TEST_HPP

