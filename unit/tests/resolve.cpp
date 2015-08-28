
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <cppunit/extensions/HelperMacros.h>

#include "../src/config.h"
#include "../src/list.h"
#include "../src/resolve.h"

class TestResolve : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TestResolve);
	CPPUNIT_TEST(testHostIPs);
	CPPUNIT_TEST_SUITE_END();

public:
	void testHostIPs();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestResolve);

void TestResolve::testHostIPs()
{
	const char *localhost4 = "localhost";
	const char *localhost_ip4 = "127.0.0.1";

	int family4 = AF_INET;
	struct rfs_list *ip4 = host_ips(localhost4, &family4);
	CPPUNIT_ASSERT(ip4 != NULL);

	bool localhost_ip4_present = false;
	{
		struct rfs_list *current = ip4;
		while (current != NULL)
		{
			struct resolved_addr *rec = (struct resolved_addr *)current->data;

			CPPUNIT_ASSERT(rec->addr_family == AF_INET);
			CPPUNIT_ASSERT(rec->ip != NULL);

			if (strcmp(rec->ip, localhost_ip4) == 0)
			{
				localhost_ip4_present = true;
			}

			current = current->next;
		}
	}
	CPPUNIT_ASSERT(localhost_ip4_present);

	destroy_list(&ip4);

#ifdef WITH_IPV6
	const char *localhost6 = "ip6-localhost";
	const char *localhost_ip6 = "::1";

	int family6 = AF_INET6;
	struct rfs_list *ip6 = host_ips(localhost4, &family6);

	if (ip6 == NULL)
	{
		family6 = AF_INET6;
		ip6 = host_ips(localhost6, &family6);
	}

	CPPUNIT_ASSERT(ip6 != NULL);

	bool localhost_ip6_present = false;
	{
		struct rfs_list *current = ip6;
		while (current != NULL)
		{
			struct resolved_addr *rec = (struct resolved_addr *)current->data;

			CPPUNIT_ASSERT(rec->addr_family == AF_INET6);
			CPPUNIT_ASSERT(rec->ip != NULL);

			if (strcmp(rec->ip, localhost_ip6) == 0)
			{
				localhost_ip6_present = true;
			}

			current = current->next;
		}
	}
	CPPUNIT_ASSERT(localhost_ip6_present);

	destroy_list(&ip6);
#endif

/* This test fails to resolve both IPv4 and IPv6 for localhost, 
 * which seems to be bug in glibc:
 *
 * https://sourceware.org/bugzilla/show_bug.cgi?id=15890
 * https://sourceware.org/git/gitweb.cgi?p=glibc.git;a=commitdiff;h=595aba70a4c676f7efaf6a012f54cd22aa189c5b
 *
 * However it's works for google.com, so test is correct and code is correct
 * Tested on Ubuntu 14.04 2015 Aug 27
 */
/*
	struct rfs_list *ip_unspec = host_ips(localhost4, NULL);

	localhost_ip4_present = false;
#ifdef WITH_IPV6
	localhost_ip6_present = false;
#endif
	{
		struct rfs_list *current = ip_unspec;
		while (current != NULL)
		{
			struct resolved_addr *rec = (struct resolved_addr *)current->data;

#ifdef WITH_IPV6
			CPPUNIT_ASSERT(rec->addr_family == AF_INET || rec->addr_family == AF_INET6);
#else
			CPPUNIT_ASSERT(rec->addr_family == AF_INET);
#endif
			CPPUNIT_ASSERT(rec->ip != NULL);

			if (strcmp(rec->ip, localhost_ip4) == 0)
			{
				localhost_ip4_present = true;
			}

#ifdef WITH_IPV6
			if (strcmp(rec->ip, localhost_ip6) == 0)
			{
				localhost_ip6_present = true;
			}
#endif

			current = current->next;
		}
	}

	CPPUNIT_ASSERT(localhost_ip4_present);
#ifdef WITH_IPV6
	CPPUNIT_ASSERT(localhost_ip6_present);
#endif

	destroy_list(&ip_unspec);
	*/
}
