
#include <stdlib.h>
#include <string.h>
#include <cppunit/extensions/HelperMacros.h>

#include "../src/config.h"
#include "../src/instance_client.h"
#include "../src/names.h"

namespace
{
#define TEST_NAME  "root"
#define TEST_HOST  "127.0.0.1"
#define WRONG_HOST "127.0.1.1"

	const char *test_name = TEST_NAME;
	const char *test_host = TEST_HOST;
	const char *test_nss_name = TEST_NAME "@" TEST_HOST;
	const char *wrong_nss_name = TEST_NAME "@" WRONG_HOST;

#undef WRONG_HOST
#undef TEST_HOST
#undef TEST_NAME
}

class TestNames : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TestNames);
	CPPUNIT_TEST(testExtract);
	CPPUNIT_TEST(testNSSCheck);
	CPPUNIT_TEST(testTransforms);
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp();
	void tearDown();

public:
	void testExtract();
	void testNSSCheck();
	void testTransforms();

protected:
	struct rfs_instance m_instance;
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestNames);

void TestNames::setUp()
{
	init_rfs_instance(&m_instance);
}

void TestNames::tearDown()
{
	release_rfs_instance(&m_instance);
}

void TestNames::testExtract()
{
	char *name = extract_name(test_nss_name);
	CPPUNIT_ASSERT(name != NULL);
	CPPUNIT_ASSERT(strcmp(name, test_name) == 0);
	free(name);

	char *host = extract_server(test_nss_name);
	CPPUNIT_ASSERT(host != NULL);
	CPPUNIT_ASSERT(strcmp(host, test_host) == 0);
	free(host);
}

void TestNames::testNSSCheck()
{
	CPPUNIT_ASSERT(is_nss_name("root@127.0.0.1") != 0);
	CPPUNIT_ASSERT(is_nss_name("root") == 0);
}

void TestNames::testTransforms()
{
	m_instance.config.host = strdup(test_host);

	char *local_name = local_nss_name(test_nss_name, &m_instance);
	CPPUNIT_ASSERT(local_name != NULL);
	CPPUNIT_ASSERT(strcmp(local_name, test_name) == 0);
	free(local_name);

	char *wrong_local_name = local_nss_name(wrong_nss_name, &m_instance);
	CPPUNIT_ASSERT(wrong_local_name == NULL);

	char *remote_name = remote_nss_name(test_name, &m_instance);
	CPPUNIT_ASSERT(remote_name != NULL);
	CPPUNIT_ASSERT(strcmp(remote_name, test_nss_name) == 0);
	free(remote_name);

	free(m_instance.config.host);
}

