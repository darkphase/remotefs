
#include <string.h>
#include <cppunit/extensions/HelperMacros.h>

#include "../src/config.h"
#include "../src/attr_cache.h"
#include "../src/instance_client.h"

class TestAttrCache : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TestAttrCache);
	CPPUNIT_TEST(testBasicCaching);
	CPPUNIT_TEST(testOutdatedCache);
	CPPUNIT_TEST_SUITE_END();

public:
	void testBasicCaching();
	void testOutdatedCache();

	virtual void setUp();
	virtual void tearDown();

protected:
	struct rfs_instance m_instance;
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestAttrCache);

void TestAttrCache::setUp()
{
	init_rfs_instance(&m_instance);
}

void TestAttrCache::tearDown()
{
	release_rfs_instance(&m_instance);
}

void TestAttrCache::testBasicCaching()
{
	struct stat stbuf = { 0 };
	const char cached_file[] = "test_attr_cache.cpp";
	const char left_file[] = "left.cpp";

	CPPUNIT_ASSERT(cache_file(&m_instance.attr_cache, cached_file, &stbuf) != NULL);
	CPPUNIT_ASSERT(cache_file(&m_instance.attr_cache, left_file, &stbuf) != NULL);

	const struct tree_item* cached = get_cache(&m_instance.attr_cache, cached_file);
	CPPUNIT_ASSERT(cached != NULL);
	CPPUNIT_ASSERT(strcmp(cached->path, cached_file) == 0);
	CPPUNIT_ASSERT(memcmp(&cached->data, &stbuf, sizeof(stbuf)) == 0);

	delete_from_cache(&m_instance.attr_cache, cached_file);
	CPPUNIT_ASSERT(get_cache(&m_instance.attr_cache, cached_file) == NULL);

	CPPUNIT_ASSERT(m_instance.attr_cache.cache != NULL);
	destroy_cache(&m_instance.attr_cache);
	CPPUNIT_ASSERT(m_instance.attr_cache.cache == NULL);
}

void TestAttrCache::testOutdatedCache()
{
	struct stat stbuf = { 0 };
	const char cached_file[] = "test_attr_cache.cpp";
	
	CPPUNIT_ASSERT(cache_file(&m_instance.attr_cache, cached_file, &stbuf) != NULL);
	DEBUG("%s\n", "waiting for cache to invalidate");
	sleep(ATTR_CACHE_TTL + 1); // wait for cache to become outdated
	CPPUNIT_ASSERT(cache_is_old(&m_instance.attr_cache) != 0);
	
	destroy_cache(&m_instance.attr_cache);
	CPPUNIT_ASSERT(m_instance.attr_cache.cache == NULL);
}

