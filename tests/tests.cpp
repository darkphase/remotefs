#include <cppunit/ui/text/TestRunner.h>

#include "test_read_cache.h"
#include "test_write_cache.h"

int main()
{
	CppUnit::TextUi::TestRunner runner;

	runner.addTest(ReadCacheTest::suite());
	runner.addTest(WriteCacheTest::suite());
	runner.run();

	return 0;
}

