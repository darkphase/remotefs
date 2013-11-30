#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <cppunit/extensions/HelperMacros.h>

#include "../src/config.h"
#include "../src/list.h"
#include "../src/resume/resume.h"

class TestResume : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TestResume);
	CPPUNIT_TEST(testOpenFiles);
	CPPUNIT_TEST(testLockedFiles);
	CPPUNIT_TEST_SUITE_END();

public:
	void testOpenFiles();
	void testLockedFiles();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestResume);

void TestResume::testOpenFiles()
{
	const char *files[] = { "./test1.cpp", "/test2.cpp", "/test/test3.cpp" };
	size_t files_count = sizeof(files) / sizeof(files[0]);

	struct rfs_list *root = NULL;

	for (size_t i = 0; i < files_count; ++i)
	{
		CPPUNIT_ASSERT(resume_add_file_to_open_list(&root, files[i], (int)(i), (uint64_t)(i)) == 0);
	}
	CPPUNIT_ASSERT(root != NULL);

	for (size_t i = 0; i < files_count; ++i)
	{
		CPPUNIT_ASSERT(resume_is_file_in_open_list(root, files[i]) == (uint64_t)(i));
	}

	const struct rfs_list *item = root;
	size_t list_pos = 0;
	while (item != NULL)
	{
		const struct open_rec *rec = (const struct open_rec *)(item->data);

		CPPUNIT_ASSERT(strcmp(rec->path, files[list_pos]) == 0);
		CPPUNIT_ASSERT(rec->flags == (int)(list_pos));
		CPPUNIT_ASSERT(rec->desc == (uint64_t)(list_pos));

		++list_pos;
		item = item->next;
	}

	for (size_t i = 0; i < files_count; ++i)
	{
		CPPUNIT_ASSERT(resume_remove_file_from_open_list(&root, files[i]) == 0);
	}
	CPPUNIT_ASSERT(root == NULL);
}

void TestResume::testLockedFiles()
{
	const char *files[] = { "./test1.cpp", "/test2.cpp", "/tests/test3.cpp" };
	size_t files_count = sizeof(files) / sizeof(files[0]);

	struct rfs_list *root = NULL;

	for (size_t i = 0; i < files_count; ++i)
	{
		CPPUNIT_ASSERT(resume_add_file_to_locked_list(&root, files[i], F_RDLCK | F_WRLCK, (unsigned)(i)) == 0);
	}
	CPPUNIT_ASSERT(root != NULL);

	const struct rfs_list *item = root;
	size_t list_pos = 0;
	while (item != NULL)
	{
		const struct lock_rec *rec = (const struct lock_rec *)(item->data);

		CPPUNIT_ASSERT(strcmp(rec->path, files[list_pos]) == 0);
		CPPUNIT_ASSERT(rec->lock_type == (F_RDLCK | F_WRLCK));
		CPPUNIT_ASSERT(rec->fully_locked == (unsigned)(list_pos));

		++list_pos;
		item = item->next;
	}

	for (size_t i = 0; i < files_count; ++i)
	{
		CPPUNIT_ASSERT(resume_remove_file_from_open_list(&root, files[i]) == 0);
	}
	CPPUNIT_ASSERT(root == NULL);
}

