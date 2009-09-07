
#include <cppunit/extensions/HelperMacros.h>

#include "config.h"
#include "id_lookup.h"

class TestIdLookup : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TestIdLookup);
	CPPUNIT_TEST(testLookup);
	CPPUNIT_TEST_SUITE_END();

public:
	void testLookup();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestIdLookup);

void TestIdLookup::testLookup()
{
	struct list *uids = NULL;
	struct list *gids = NULL;

	CPPUNIT_ASSERT(create_uids_lookup(&uids) == 0);
	CPPUNIT_ASSERT(create_gids_lookup(&gids) == 0);
	CPPUNIT_ASSERT(uids != NULL && gids != NULL);

	const char *whoami = get_uid_name(uids, getuid());
	const char *mygroup = get_gid_name(gids, getgid());
	const char *root = get_uid_name(uids, 0);
	const char *root_group = get_gid_name(gids, 0);
	
	CPPUNIT_ASSERT(whoami != NULL && mygroup != NULL);
	CPPUNIT_ASSERT(root != NULL && root_group != NULL);

	CPPUNIT_ASSERT(get_uid(uids, whoami) == getuid());
	CPPUNIT_ASSERT(get_gid(gids, mygroup) == getgid());
	CPPUNIT_ASSERT(get_uid(uids, root) == 0);
	CPPUNIT_ASSERT(get_gid(gids, root_group) == 0);

	destroy_uids_lookup(&uids);
	destroy_gids_lookup(&gids);
	CPPUNIT_ASSERT(uids == NULL && gids == NULL);
}

