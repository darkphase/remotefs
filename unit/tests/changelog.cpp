
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <cppunit/extensions/HelperMacros.h>

#include "../src/config.h"
#include "../src/changelog.h"

class TestChangelog : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TestChangelog);
	CPPUNIT_TEST(testVersionConvert);
	CPPUNIT_TEST(testCompatibility);
	CPPUNIT_TEST_SUITE_END();

public:
	void testVersionConvert();
	void testCompatibility();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestChangelog);

void TestChangelog::testVersionConvert()
{
	CPPUNIT_ASSERT(COMPAT_VERSION(0, 1) < COMPAT_VERSION(0, 2));
	CPPUNIT_ASSERT(COMPAT_VERSION(0, 99) < COMPAT_VERSION(1, 0));
}

void TestChangelog::testCompatibility()
{
	// 0.5 should be marked as incompatible to 0.1-0.4
	// 0.6 should be marked as incompatible to 0.5

	CPPUNIT_ASSERT(versions_compatible(
	COMPAT_VERSION(0, 1), 
	COMPAT_VERSION(0, 1)) != 0);

	CPPUNIT_ASSERT(versions_compatible(
	COMPAT_VERSION(0, 1), 
	COMPAT_VERSION(0, 2)) != 0);

	CPPUNIT_ASSERT(versions_compatible(
	COMPAT_VERSION(0, 2), 
	COMPAT_VERSION(0, 1)) != 0);

	CPPUNIT_ASSERT(versions_compatible(
	COMPAT_VERSION(0, 4), 
	COMPAT_VERSION(0, 4)) != 0);

	CPPUNIT_ASSERT(versions_compatible(
	COMPAT_VERSION(0, 5), 
	COMPAT_VERSION(0, 5)) != 0);

	CPPUNIT_ASSERT(versions_compatible(
	COMPAT_VERSION(0, 1), 
	COMPAT_VERSION(0, 5)) == 0);

	CPPUNIT_ASSERT(versions_compatible(
	COMPAT_VERSION(0, 5), 
	COMPAT_VERSION(0, 1)) == 0);

	CPPUNIT_ASSERT(versions_compatible(
	COMPAT_VERSION(0, 4), 
	COMPAT_VERSION(0, 5)) == 0);

	CPPUNIT_ASSERT(versions_compatible(
	COMPAT_VERSION(0, 5),
	COMPAT_VERSION(0, 4)) == 0);

	CPPUNIT_ASSERT(versions_compatible(
	COMPAT_VERSION(0, 6), 
	COMPAT_VERSION(0, 6)) != 0);
}
