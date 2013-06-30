
#include <stdlib.h>
#include <string.h>
#include <cppunit/extensions/HelperMacros.h>

#include "../src/config.h"
#include "../src/crypt.h"

namespace
{
	const char *empty_pass = "qRPK7m23GJusamGpoGLby/";
}

class TestCrypt : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TestCrypt);
	CPPUNIT_TEST(testPasswdHash);
	CPPUNIT_TEST_SUITE_END();

public:
	void testPasswdHash();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestCrypt);

void TestCrypt::testPasswdHash()
{
	char *empty_hash = passwd_hash("", EMPTY_SALT);
	CPPUNIT_ASSERT(empty_hash != NULL);
	CPPUNIT_ASSERT(strcmp(empty_hash, empty_pass) == 0);

	char *notempty_hash = passwd_hash(" ", EMPTY_SALT);
	CPPUNIT_ASSERT(notempty_hash != NULL);
	CPPUNIT_ASSERT(strcmp(empty_hash, notempty_hash) != 0);

	free(notempty_hash);
	free(empty_hash);
}

