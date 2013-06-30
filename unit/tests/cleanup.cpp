#include <string.h>
#include <cppunit/extensions/HelperMacros.h>

#include "../src/config.h"
#include "../src/list.h"
#include "../src/resume/cleanup.h"

class TestCleanup : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TestCleanup);
	CPPUNIT_TEST(testCleanup);
	CPPUNIT_TEST_SUITE_END();

public:
	void testCleanup();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestCleanup);

void TestCleanup::testCleanup()
{
	struct list *open_files = NULL;
	int file_desc = 1;
	int file_desc_left = 2;

	CPPUNIT_ASSERT(cleanup_add_file_to_open_list(&open_files, file_desc) == 0);
	CPPUNIT_ASSERT(cleanup_add_file_to_open_list(&open_files, file_desc_left) == 0);
	CPPUNIT_ASSERT(open_files != NULL);
	CPPUNIT_ASSERT(*(int *)(open_files->data) == file_desc);
	CPPUNIT_ASSERT(cleanup_remove_file_from_open_list(&open_files, file_desc) == 0);
	CPPUNIT_ASSERT(*(int *)(open_files->data) == file_desc_left);
	CPPUNIT_ASSERT(cleanup_files(&open_files) == 0);
	CPPUNIT_ASSERT(open_files == NULL);
}

