
#include <errno.h>
#include <cppunit/extensions/HelperMacros.h>

#include "../src/config.h"
#include "../src/error.h"

class TestError : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TestError);
	CPPUNIT_TEST(testHTON);
	CPPUNIT_TEST_SUITE_END();

public:
	void testHTON();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestError);

void TestError::testHTON()
{
	CPPUNIT_ASSERT(E2BIG == ntoh_errno(hton_errno(E2BIG)));
	CPPUNIT_ASSERT(ntoh_errno(hton_errno(-1)) == EIO); // not POSIX.1 error should be defaulted to EIO
}

