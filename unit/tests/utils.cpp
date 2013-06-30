
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <cppunit/extensions/HelperMacros.h>

#include "../src/config.h"
#include "../src/list.h"
#include "../src/utils.h"

class TestUtils : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TestUtils);
	CPPUNIT_TEST(testCompareNetmaskIPv4);
	CPPUNIT_TEST(testIsIPv4Addr);
	CPPUNIT_TEST(testIsIPv4Local);
#ifdef WITH_IPV6
	CPPUNIT_TEST(testCompareNetmaskIPv6);
	CPPUNIT_TEST(testIsIPv6Addr);
	CPPUNIT_TEST(testIsIPv6Local);
#endif // WITH_IPV6
	CPPUNIT_TEST_SUITE_END();

public:
	void testCompareNetmaskIPv4();
	void testIsIPv4Addr();
	void testIsIPv4Local();
#ifdef WITH_IPV6
	void testCompareNetmaskIPv6();
	void testIsIPv6Addr();
	void testIsIPv6Local();
#endif // WITH_IPV6
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestUtils);

void TestUtils::testCompareNetmaskIPv4()
{
	CPPUNIT_ASSERT(compare_netmask("1.2.3.4", "1.2.3.4", 32) != 0);
	CPPUNIT_ASSERT(compare_netmask("1.2.3.4", "1.2.3.4", 24) != 0);
	CPPUNIT_ASSERT(compare_netmask("1.2.3.4", "1.2.3.0", 24) != 0);
	CPPUNIT_ASSERT(compare_netmask("1.2.3.4", "1.2.3.4", 16) != 0);
	CPPUNIT_ASSERT(compare_netmask("1.2.3.4", "1.2.0.0", 16) != 0);
	CPPUNIT_ASSERT(compare_netmask("1.2.3.4", "1.2.3.4", 8) != 0);
	CPPUNIT_ASSERT(compare_netmask("1.2.3.4", "1.0.0.0", 8) != 0);
	CPPUNIT_ASSERT(compare_netmask("1.2.3.4", "1.2.3.4", 0) != 0);
	CPPUNIT_ASSERT(compare_netmask("1.2.3.4", "0.0.0.0", 0) != 0);

	CPPUNIT_ASSERT(compare_netmask("2.2.3.4", "1.2.3.4", 32) == 0);
	CPPUNIT_ASSERT(compare_netmask("2.2.3.4", "1.2.3.4", 24) == 0);
	CPPUNIT_ASSERT(compare_netmask("2.2.3.4", "1.2.3.0", 24) == 0);
	CPPUNIT_ASSERT(compare_netmask("2.2.3.4", "1.2.3.4", 16) == 0);
	CPPUNIT_ASSERT(compare_netmask("2.2.3.4", "1.2.0.0", 16) == 0);
	CPPUNIT_ASSERT(compare_netmask("2.2.3.4", "1.2.3.4", 8) == 0);
	CPPUNIT_ASSERT(compare_netmask("2.2.3.4", "1.0.0.0", 8) == 0);
	CPPUNIT_ASSERT(compare_netmask("2.2.3.4", "1.2.3.4", 0) != 0);
	CPPUNIT_ASSERT(compare_netmask("2.2.3.4", "0.0.0.0", 0) != 0);

	CPPUNIT_ASSERT(compare_netmask("64.0.0.0", "255.0.0.0", 1) == 0);
	CPPUNIT_ASSERT(compare_netmask("128.0.0.0", "255.0.0.0", 1) != 0);
	CPPUNIT_ASSERT(compare_netmask("192.0.0.0", "255.0.0.0", 1) != 0);

	CPPUNIT_ASSERT(compare_netmask("160.0.0.0", "255.0.0.0", 2) == 0);
	CPPUNIT_ASSERT(compare_netmask("192.0.0.0", "255.0.0.0", 2) != 0);
	CPPUNIT_ASSERT(compare_netmask("224.0.0.0", "255.0.0.0", 2) != 0);
	
	CPPUNIT_ASSERT(compare_netmask("208.0.0.0", "255.0.0.0", 3) == 0);
	CPPUNIT_ASSERT(compare_netmask("224.0.0.0", "255.0.0.0", 3) != 0);
	CPPUNIT_ASSERT(compare_netmask("240.0.0.0", "255.0.0.0", 3) != 0);
	
	CPPUNIT_ASSERT(compare_netmask("127.0.0.1", "127.0.0.2", 32) == 0);

	CPPUNIT_ASSERT(compare_netmask("127.0.0.1", "0.0.0.0", 0) != 0);
	CPPUNIT_ASSERT(compare_netmask("128.0.0.1", "0.0.0.0", 1) == 0);
}

#ifdef WITH_IPV6
void TestUtils::testCompareNetmaskIPv6()
{
	CPPUNIT_ASSERT(compare_netmask("ffff::127.0.0.1", "ffff::127.0.0.1", 128) != 0);
	CPPUNIT_ASSERT(compare_netmask("::1", "::1", 128) != 0);
	CPPUNIT_ASSERT(compare_netmask("ffff::1", "ffff::", 120) != 0);
	
	CPPUNIT_ASSERT(compare_netmask("::1", "::2", 128) == 0);
	CPPUNIT_ASSERT(compare_netmask("ffff::1", "ffff::", 128) == 0);
	
	CPPUNIT_ASSERT(compare_netmask("::1", "::", 0) != 0);
}
#endif

void TestUtils::testIsIPv4Addr()
{
	CPPUNIT_ASSERT(is_ipaddr("127.0.0.1") != 0);
	CPPUNIT_ASSERT(is_ipaddr("195.177.0.1") != 0);

	CPPUNIT_ASSERT(is_ipaddr("alex") == 0);
	CPPUNIT_ASSERT(is_ipaddr("o.o.o.o") == 0);
}

#ifdef WITH_IPV6
void TestUtils::testIsIPv6Addr()
{
	CPPUNIT_ASSERT(is_ipaddr("ffff::") != 0);
	CPPUNIT_ASSERT(is_ipaddr("::1") != 0);
	CPPUNIT_ASSERT(is_ipaddr("ffff::1") != 0);

	CPPUNIT_ASSERT(is_ipaddr("alex::1") == 0);
	CPPUNIT_ASSERT(is_ipaddr("o::o::o::o") == 0);
}
#endif

void TestUtils::testIsIPv4Local()
{
	CPPUNIT_ASSERT(is_ipv4_local("127.0.0.1") != 0);
	CPPUNIT_ASSERT(is_ipv4_local("127.0.0.2") != 0);
	CPPUNIT_ASSERT(is_ipv4_local("10.0.0.1") != 0);
	CPPUNIT_ASSERT(is_ipv4_local("10.0.0.2") != 0);
	CPPUNIT_ASSERT(is_ipv4_local("10.1.0.2") != 0);
	CPPUNIT_ASSERT(is_ipv4_local("192.168.0.1") != 0);
	CPPUNIT_ASSERT(is_ipv4_local("192.168.1.1") != 0);
	CPPUNIT_ASSERT(is_ipv4_local("172.16.0.1") != 0);
	CPPUNIT_ASSERT(is_ipv4_local("172.16.1.1") != 0);
	CPPUNIT_ASSERT(is_ipv4_local("1.0.0.1") != 0);
	CPPUNIT_ASSERT(is_ipv4_local("1.1.0.1") != 0);
	CPPUNIT_ASSERT(is_ipv4_local("2.0.0.1") != 0);
	CPPUNIT_ASSERT(is_ipv4_local("2.2.0.1") != 0);
	
	CPPUNIT_ASSERT(is_ipv4_local("127.0.1.1") == 0);
	CPPUNIT_ASSERT(is_ipv4_local("11.0.0.1") == 0);
	CPPUNIT_ASSERT(is_ipv4_local("192.169.0.1") == 0);
	CPPUNIT_ASSERT(is_ipv4_local("193.168.0.1") == 0);
	CPPUNIT_ASSERT(is_ipv4_local("173.16.0.1") == 0);
	CPPUNIT_ASSERT(is_ipv4_local("172.17.0.1") == 0);
	CPPUNIT_ASSERT(is_ipv4_local("3.0.0.1") == 0);
}

#ifdef WITH_IPV6
void TestUtils::testIsIPv6Local()
{
	CPPUNIT_ASSERT(is_ipv6_local("::1") != 0);
	CPPUNIT_ASSERT(is_ipv6_local("ffff::127.0.0.1") != 0);
	CPPUNIT_ASSERT(is_ipv6_local("fe00") != 0);
	CPPUNIT_ASSERT(is_ipv6_local("feff:") != 0);

	CPPUNIT_ASSERT(is_ipv6_local("ffff::195.177.0.1") == 0);
	CPPUNIT_ASSERT(is_ipv6_local("ffff") == 0);
}
#endif
