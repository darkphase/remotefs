
#include <cppunit/extensions/HelperMacros.h>

#include "../src/config.h"
#include "../src/inet.h"

class TestInet : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TestInet);
	CPPUNIT_TEST(testHTON);
	CPPUNIT_TEST_SUITE_END();

public:
	void testHTON();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestInet);

void TestInet::testHTON()
{
	uint16_t u16 = 1;
	uint32_t u32 = 2;
	uint64_t u64 = 3;

	CPPUNIT_ASSERT(u16 == ntohs(htons(u16)));
	CPPUNIT_ASSERT(u32 == ntohl(htonl(u32)));
	CPPUNIT_ASSERT(u64 == ntohll(htonll(u64)));
}

