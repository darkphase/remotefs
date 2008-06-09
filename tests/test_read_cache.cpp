#include "test_read_cache.h"

extern "C"
{
#include "../src/read_cache.h"
#include "../src/config.h"
}

void ReadCacheTest::setUp()
{
	destroy_read_cache();
}

void ReadCacheTest::tearDown()
{
	destroy_read_cache();
}

void ReadCacheTest::testBasics()
{
	char buffer[4] = { 0, 1, 2, 3 };

	off_t offset = 0;
	size_t size = sizeof(buffer);
	uint64_t descriptor = 0;
	uint64_t wrong_descriptor = descriptor + 1;

	CPPUNIT_ASSERT(read_cache_max_size() >= size);
	CPPUNIT_ASSERT(put_to_read_cache(descriptor, buffer, size, offset) == 0);

	CPPUNIT_ASSERT(read_cache_have_data(descriptor, offset) == size);
	CPPUNIT_ASSERT(read_cache_have_data(descriptor, 1) == size - 1);
	CPPUNIT_ASSERT(read_cache_have_data(descriptor, size - 1) == 1);
	CPPUNIT_ASSERT(read_cache_have_data(descriptor, size) == 0);
	
	CPPUNIT_ASSERT(read_cache_have_data(wrong_descriptor, offset) == 0);

	for (size_t i = 0; i < size; ++i)
	{
		CPPUNIT_ASSERT(read_cache_get_data(descriptor, size - i, i) != NULL);
		CPPUNIT_ASSERT(memcmp(read_cache_get_data(descriptor, size - i, i), buffer + i, size - i) == 0);
	}
	
	CPPUNIT_ASSERT(read_cache_get_data(descriptor, size, offset) != NULL);
	CPPUNIT_ASSERT(read_cache_get_data(descriptor, size + 1, offset) == NULL);
	CPPUNIT_ASSERT(read_cache_get_data(descriptor, size, offset + 1) == NULL);
}

void ReadCacheTest::testMultipleFiles()
{
	char buffer[1024] = { 0 };
	uint64_t descriptor = 0;
	uint64_t another_descriptor = 1;
	size_t size = sizeof(buffer);
	off_t offset = 0;

	CPPUNIT_ASSERT(read_cache_max_size() >= size);
	CPPUNIT_ASSERT(put_to_read_cache(descriptor, buffer, size, offset) == 0);

	CPPUNIT_ASSERT(read_cache_have_data(descriptor, offset) == size);
	CPPUNIT_ASSERT(read_cache_have_data(another_descriptor, offset) == 0);

	CPPUNIT_ASSERT(put_to_read_cache(another_descriptor, buffer, size, offset) != 0);
	
	destroy_read_cache();

	CPPUNIT_ASSERT(put_to_read_cache(another_descriptor, buffer, size, offset) == 0);
	CPPUNIT_ASSERT(read_cache_have_data(another_descriptor, offset) == size);
	CPPUNIT_ASSERT(read_cache_have_data(descriptor, offset) == 0);
	
	CPPUNIT_ASSERT(put_to_read_cache(descriptor, buffer, size, offset) != 0);
}

void ReadCacheTest::testMaxSize()
{
	char buffer[DEFAULT_RW_CACHE_SIZE * 2] = { 0 };
	size_t size = sizeof(buffer);

	CPPUNIT_ASSERT(size > read_cache_max_size());
	CPPUNIT_ASSERT(put_to_read_cache(0, NULL, size, 0) != 0);
}

void ReadCacheTest::testCleanup()
{
	char buffer[1024] = { 0 };
	size_t size = sizeof(buffer);
	uint64_t descriptor = 0;
	
	CPPUNIT_ASSERT(put_to_read_cache(descriptor, buffer, size, 0) == 0);
	CPPUNIT_ASSERT(read_cache_get_data(descriptor, size, 0) != NULL);
	CPPUNIT_ASSERT(read_cache_size(descriptor) == size);
	
	destroy_read_cache();
	
	CPPUNIT_ASSERT(read_cache_get_data(descriptor, size, 0) == NULL);
	CPPUNIT_ASSERT(read_cache_size(descriptor) == 0);
}
