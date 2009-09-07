
#include <string.h>
#include <cppunit/extensions/HelperMacros.h>

#include "config.h"
#include "buffer.h"

class TestBuffer : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TestBuffer);
	CPPUNIT_TEST(testPacking);
	CPPUNIT_TEST(testDup);
	CPPUNIT_TEST_SUITE_END();

public:
	void testPacking();
	void testDup();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestBuffer);

void TestBuffer::testPacking()
{
	uint16_t u16 = 1, u16b = 0;
	uint32_t u32 = 2, u32b = 0;
	uint64_t u64 = 3, u64b = 0;
	int16_t  i16 = 4, i16b = 0;
	int32_t  i32 = 5, i32b = 0;
	int64_t  i64 = 6, i64b = 0;

	size_t buffer_size = sizeof(uint64_t) + sizeof(uint32_t) + sizeof(uint16_t);
	char *buffer = (char *)get_buffer(buffer_size);
	CPPUNIT_ASSERT(buffer != NULL);

	CPPUNIT_ASSERT(
	pack_16(&u16, 
	pack_32(&u32, 
	pack_64(&u64, buffer)))
	== buffer + buffer_size);

	CPPUNIT_ASSERT(
	unpack_16(&u16b, 
	unpack_32(&u32b, 
	unpack_64(&u64b, buffer)))
	== buffer + buffer_size);

	CPPUNIT_ASSERT(u16 == u16b && u32 == u32b && u64 == u64b);

	CPPUNIT_ASSERT(
	pack_16_s(&i16, 
	pack_32_s(&i32, 
	pack_64_s(&i64, buffer)))
	== buffer + buffer_size);

	CPPUNIT_ASSERT(
	unpack_16_s(&i16b, 
	unpack_32_s(&i32b, 
	unpack_64_s(&i64b, buffer)))
	== buffer + buffer_size);

	CPPUNIT_ASSERT(i16 == i16b && i32 == i32b && i64 == i64b);

	free_buffer(buffer);
}

void TestBuffer::testDup()
{
	const char str[] = "abcdef";
	size_t buffer_size = strlen(str);

	char *buffer = (char *)get_buffer(buffer_size);
	CPPUNIT_ASSERT(buffer != NULL);
	
	memcpy(buffer, str, buffer_size);

	char *dup = buffer_dup(buffer, buffer_size);
	CPPUNIT_ASSERT(dup != NULL);
	CPPUNIT_ASSERT(memcmp(buffer, dup, buffer_size) == 0);
	free_buffer(dup);

	size_t str_dup_size = buffer_size / 2;
	char *str_dup = buffer_dup_str(buffer, str_dup_size);
	
	CPPUNIT_ASSERT(str_dup != NULL);
	CPPUNIT_ASSERT(strlen(str_dup) == str_dup_size);
	CPPUNIT_ASSERT(memcmp(buffer, str_dup, str_dup_size) == 0);
	free_buffer(str_dup);

	free_buffer(buffer);
}

