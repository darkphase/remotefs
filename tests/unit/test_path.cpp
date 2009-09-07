
#include <errno.h>
#include <string.h>
#include <cppunit/extensions/HelperMacros.h>

#include "config.h"
#include "path.h"

class TestPath : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TestPath);
	CPPUNIT_TEST(testJoin);
	CPPUNIT_TEST(testBounds);
	CPPUNIT_TEST(testDots);
	CPPUNIT_TEST_SUITE_END();

public:
	void testJoin();
	void testBounds();
	void testDots();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestPath);

void TestPath::testJoin()
{
	char dir[] = "/etc";
	char slashed_dir[] = "/etc/";
	char naked_dir[] = "etc";
	char file[] = "hosts";

	char full_path[FILENAME_MAX + 1];
	char slashed_full_path[FILENAME_MAX + 1];
	char naked_full_path[FILENAME_MAX + 1];

	int full_path_len = path_join(full_path, sizeof(full_path), dir, file);
	int slashed_path_len = path_join(slashed_full_path, sizeof(slashed_full_path), slashed_dir, file);
	int naked_path_len = path_join(naked_full_path, sizeof(naked_full_path), naked_dir, file);

	CPPUNIT_ASSERT(full_path_len > 0 && slashed_path_len > 0 && naked_path_len > 0);

	CPPUNIT_ASSERT(slashed_path_len == strlen(slashed_dir) + strlen(file));
	CPPUNIT_ASSERT(slashed_path_len == strlen(slashed_full_path));
	CPPUNIT_ASSERT(memcmp(slashed_full_path, slashed_dir, strlen(slashed_dir)) == 0);
	CPPUNIT_ASSERT(memcmp(slashed_full_path + strlen(slashed_dir), file, strlen(file)) == 0);

	CPPUNIT_ASSERT(strlen(full_path) == strlen(slashed_full_path) && full_path_len == slashed_path_len);
	CPPUNIT_ASSERT(strlen(full_path) == strlen(naked_full_path) + 1 && full_path_len == naked_path_len + 1);

	CPPUNIT_ASSERT(memcmp(full_path, slashed_full_path, strlen(full_path)) == 0);
	CPPUNIT_ASSERT(memcmp(full_path + 1, naked_full_path, strlen(naked_full_path)) == 0);
}

void TestPath::testBounds()
{
	char dir[] = "/etc";
	char file[] = "hosts";

	size_t buffer_size = strlen(dir) + strlen(file); // no space for slash
	char *full_path = new char[buffer_size];
	CPPUNIT_ASSERT(path_join(full_path, buffer_size, dir, file) == -E2BIG);
	delete[] full_path;

	buffer_size = strlen(dir) + strlen(file) + 1; // still no space for \0
	full_path = new char[buffer_size];
	CPPUNIT_ASSERT(path_join(full_path, buffer_size, dir, file) == -E2BIG);
	delete[] full_path;
	
	buffer_size = strlen(dir) + strlen(file) + 2; // should be just enough space
	full_path = new char[buffer_size];
	CPPUNIT_ASSERT(path_join(full_path, buffer_size, dir, file) == buffer_size - 1);
	delete[] full_path;
}

void TestPath::testDots()
{
	char dir[] = "/etc/";
	char dot[] = ".";
	char dotdot[] = "..";

	char full_path[FILENAME_MAX + 1];
	
	int full_path_len = path_join(full_path, sizeof(full_path), dir, dot);
	CPPUNIT_ASSERT(full_path_len == strlen(dir) + strlen(dot));
	CPPUNIT_ASSERT(memcmp(full_path, dir, strlen(dir)) == 0);
	CPPUNIT_ASSERT(memcmp(full_path + strlen(dir), dot, strlen(dot)) == 0);
	
	full_path_len = path_join(full_path, sizeof(full_path), dir, dotdot);
	CPPUNIT_ASSERT(full_path_len == strlen(dir) + strlen(dotdot));
	CPPUNIT_ASSERT(memcmp(full_path, dir, strlen(dir)) == 0);
	CPPUNIT_ASSERT(memcmp(full_path + strlen(dir), dotdot, strlen(dotdot)) == 0);
}

