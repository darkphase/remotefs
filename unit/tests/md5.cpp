
#include <stdlib.h>
#include <string.h>
#include <cppunit/extensions/HelperMacros.h>

#include "../src/config.h"
#include "../src/md5crypt/md5.h"

namespace
{
	const char *empty_md5 = "d41d8cd98f00b204e9800998ecf8427e";

	void to_string(unsigned char digest[16], char str[33])
	{
		for (size_t i = 0; i < 16; ++i)
		{
			snprintf(str + i * 2, 33 - i, "%02x", digest[i]);
		}
	}
}

class TestMD5 : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TestMD5);
	CPPUNIT_TEST(testMD5);
	CPPUNIT_TEST_SUITE_END();

public:
	void testMD5();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestMD5);

void TestMD5::testMD5()
{
	MD5_CTX context = { { 0 } };
	unsigned char digest[16] = { 0 };

	MD5Init(&context);
	MD5Final(digest, &context);

	char empty_digest[33] = { 0 };
	to_string(digest, empty_digest);
	CPPUNIT_ASSERT(strcmp(empty_digest, empty_md5) == 0);

	MD5Init(&context);
	MD5Update(&context, (unsigned char *)" ", 1);
	MD5Final(digest, &context);

	char notempty_digest[33] = { 0 };
	to_string(digest, notempty_digest);
	CPPUNIT_ASSERT(strcmp(notempty_digest, empty_md5) != 0);
}

