#ifndef FRAMEWORK_TEST_HPP
#define FRAMEWORK_TEST_HPP

#include <cppunit/extensions/HelperMacros.h>

class FrameworkTest : public CppUnit::TestFixture
{
public:
	virtual void setUp();
	virtual void tearDown();
};

#endif // FRAMEWORK_TEST_HPP

