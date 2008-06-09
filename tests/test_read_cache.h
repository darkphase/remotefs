#ifndef TEST_READ_CACHE_H
#define TEST_READ_CACHE_H

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

class ReadCacheTest : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(ReadCacheTest);
		CPPUNIT_TEST(testBasics);
		CPPUNIT_TEST(testMultipleFiles);
		CPPUNIT_TEST(testMaxSize);
		CPPUNIT_TEST(testCleanup);
	CPPUNIT_TEST_SUITE_END();

public:
	virtual void setUp();
	virtual void tearDown();

	void testBasics();
	void testMultipleFiles();
	void testMaxSize();
	void testCleanup();
};

#endif // TEST_READ_CACHE_H

