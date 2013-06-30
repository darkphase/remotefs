
#include <string.h>
#include <cppunit/extensions/HelperMacros.h>

#include "../src/config.h"
#include "../src/instance_client.h"
#include "../src/operations/operations.h"

class TestTransformSymlink : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TestTransformSymlink);
	CPPUNIT_TEST(testTransform);
	CPPUNIT_TEST_SUITE_END();

public:
	void testTransform();

	virtual void setUp();
	virtual void tearDown();

protected:
	struct rfs_instance m_instance;
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestTransformSymlink);

void TestTransformSymlink::setUp()
{
	init_rfs_instance(&m_instance);
	m_instance.config.path = strdup("/mnt/usb");
}

void TestTransformSymlink::tearDown()
{
	release_rfs_instance(&m_instance);
}

void TestTransformSymlink::testTransform()
{
	CPPUNIT_ASSERT(_transform_symlink(&m_instance, "mnt/usb", "mnt/usb/store") == NULL); /* symlinks w/o heading '/' shouldn't be transformed */
	
	CPPUNIT_ASSERT(strcmp(_transform_symlink(&m_instance, "/store", "/mnt/usb/store.link"), "store.link") == 0);
	CPPUNIT_ASSERT(strcmp(_transform_symlink(&m_instance, "/store/repo", "/mnt/usb/repo/repo.link"), "../repo/repo.link") == 0);
	CPPUNIT_ASSERT(strcmp(_transform_symlink(&m_instance, "/store/repo", "/mnt/usb/store/repo/repo.link"), "../store/repo/repo.link") == 0);
}

