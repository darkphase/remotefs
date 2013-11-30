
#include <cppunit/extensions/HelperMacros.h>

#include "../src/config.h"
#include "../src/list.h"

class TestList : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TestList);
	CPPUNIT_TEST(testList);
	CPPUNIT_TEST_SUITE_END();

public:
	void testList();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestList);

void TestList::testList()
{
	struct rfs_list *root = NULL;
	size_t range_start = 0;
	size_t range_stop = 10;

	for (size_t i = range_start; i < range_stop; ++i)
	{
		CPPUNIT_ASSERT(add_to_list(&root, new size_t(i)) != NULL);
	}

	CPPUNIT_ASSERT(list_length(root) == (range_stop - range_start));

	struct rfs_list *item = root;
	size_t current = 0;
	while (item != NULL)
	{
		CPPUNIT_ASSERT(*static_cast<size_t *>(item->data) == current);

		++current;
		item = item->next;
	}

	struct rfs_list *new_root = root->next;
	size_t *old_root = static_cast<size_t *>(extract_from_list(&root, root));
	CPPUNIT_ASSERT(*old_root == range_start);
	delete old_root;

	CPPUNIT_ASSERT(root == new_root);
	new_root = root->next;
	CPPUNIT_ASSERT(remove_from_list(&root, root) == new_root);
	CPPUNIT_ASSERT(root == new_root);

	destroy_list(&root);
	CPPUNIT_ASSERT(root == NULL);
}

