#include "test_write_cache.h"

extern "C"
{
#include "../src/write_cache.h"
#include "../src/config.h"
}

void WriteCacheTest::setUp()
{
	destroy_write_cache();
}

void WriteCacheTest::tearDown()
{
	destroy_write_cache();
}

void WriteCacheTest::testBasics()
{
	char buffer[DEFAULT_RW_CACHE_SIZE] = { 0 };
	uint64_t descriptor = 0;
	size_t size = sizeof(buffer);
	off_t offset = 0;
	const char *path = "";
	
	CPPUNIT_ASSERT(init_write_cache(path, offset, size / 2) == 0);
	
	CPPUNIT_ASSERT(is_fit_to_write_cache(descriptor, size, offset) == 0);
	CPPUNIT_ASSERT(is_fit_to_write_cache(descriptor, size / 2, offset) != 0);
	
	CPPUNIT_ASSERT(add_to_write_cache(descriptor, buffer, size / 4, offset) == 0);
	CPPUNIT_ASSERT(is_fit_to_write_cache(descriptor, size / 4, offset + size / 2) == 0);
	CPPUNIT_ASSERT(is_fit_to_write_cache(descriptor, size / 4, offset + size / 4) != 0);
	
	CPPUNIT_ASSERT(is_fit_to_write_cache(descriptor + 1, size / 4, offset + size / 4) == 0);
	CPPUNIT_ASSERT(is_fit_to_write_cache(descriptor, size / 4, offset) == 0);
	
	CPPUNIT_ASSERT(get_write_cache_size() == size / 4);
	CPPUNIT_ASSERT(add_to_write_cache(descriptor, buffer + size / 4, size / 4, offset + size / 4) == 0);
	CPPUNIT_ASSERT(get_write_cache_size() == size / 2);
	CPPUNIT_ASSERT(get_write_cache_block() != NULL);
	
	CPPUNIT_ASSERT(is_fit_to_write_cache(descriptor, 1, offset + size / 2) == 0);
}

void WriteCacheTest::testMultipleFiles()
{
	char buffer[1024] = { 0 };
	uint64_t descriptor = 0;
	uint64_t another_descriptor = 1;
	size_t size = sizeof(buffer);
	off_t offset = 0;
	unsigned pieces = 4;
	const char *path = "";
	
	CPPUNIT_ASSERT(init_write_cache(path, offset, size * pieces) == 0);
	
	for (unsigned i = 0; i < pieces; ++i)
	{
		CPPUNIT_ASSERT(is_fit_to_write_cache(descriptor, size, offset + size * i) != 0);
		CPPUNIT_ASSERT(add_to_write_cache(descriptor, buffer, size, offset + size * i) == 0);
	}
	
	CPPUNIT_ASSERT(is_fit_to_write_cache(another_descriptor, size, offset) == 0);
	CPPUNIT_ASSERT(add_to_write_cache(another_descriptor, buffer, size, offset) != 0);
	
	destroy_write_cache();
	uninit_write_cache();
	
	CPPUNIT_ASSERT(init_write_cache(path, offset, size * pieces) == 0);
	
	for (unsigned i = 0; i < pieces; ++i)
	{
		CPPUNIT_ASSERT(is_fit_to_write_cache(another_descriptor, size, offset + size * i) != 0);
		CPPUNIT_ASSERT(add_to_write_cache(another_descriptor, buffer, size, offset + size * i) == 0);
	}
	
	CPPUNIT_ASSERT(is_fit_to_write_cache(descriptor, size, offset) == 0);
	CPPUNIT_ASSERT(add_to_write_cache(descriptor, buffer, size, offset) != 0);
}

void WriteCacheTest::testMaxSize()
{
	char buffer[DEFAULT_RW_CACHE_SIZE] = { 0 };
	uint64_t descriptor = 0;
	size_t size = sizeof(buffer);
	off_t offset = 0;
	const char *path = "";
	
	CPPUNIT_ASSERT(init_write_cache(path, offset, size / 2) == 0);
	
	CPPUNIT_ASSERT(is_fit_to_write_cache(descriptor, size, offset) == 0);
	CPPUNIT_ASSERT(add_to_write_cache(descriptor, buffer, size, offset) != 0);
}

void WriteCacheTest::testCleanup()
{
	char buffer[1024] = { 0 };
	uint64_t descriptor = 0;
	size_t size = sizeof(buffer);
	off_t offset = 0;
	const char *path = "";
	
	CPPUNIT_ASSERT(init_write_cache(path, offset, size) == 0);
	
	CPPUNIT_ASSERT(is_fit_to_write_cache(descriptor, size, offset) != 0);
	CPPUNIT_ASSERT(add_to_write_cache(descriptor, buffer, size, offset) == 0);
	CPPUNIT_ASSERT(get_write_cache_block() != NULL);
	CPPUNIT_ASSERT(get_write_cache_size() == size);
	
	destroy_write_cache();
	
	CPPUNIT_ASSERT(get_write_cache_block() == NULL);
	CPPUNIT_ASSERT(get_write_cache_size() == 0);
}
