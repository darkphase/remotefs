
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <cppunit/extensions/HelperMacros.h>

#include "config.h"
#include "exports.h"
#include "list.h"

class TestExports : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TestExports);
	CPPUNIT_TEST(testCorrectParsing);
	CPPUNIT_TEST(testWrongParsing);
	CPPUNIT_TEST(testDupesParsing);
	CPPUNIT_TEST(testEmptyParsing);
	CPPUNIT_TEST_SUITE_END();

public:
	virtual void tearDown();
	void testCorrectParsing();
	void testWrongParsing();
	void testDupesParsing();
	void testEmptyParsing();

protected:
	static const char *exportsfile;
};

const char* TestExports::exportsfile = "./rfs-exports";

CPPUNIT_TEST_SUITE_REGISTRATION(TestExports);

void TestExports::tearDown()
{
	unlink(exportsfile);
}

void TestExports::testCorrectParsing()
{
	// need to avoid duplicate entries in exports paths
	const char *correct_exports[] = 
		{ 
		"/var root, 127.0.0.1 ()", 
		"/bin root, 127.0.0.1", 
		"/tmp root (ugo)", 
		"/sbin 127.0.0.1 ( ro)", 
		"/home/alex root (user=root )", 
		"/home 127.0.0.1,root( ro,user=root )", 
		" /usr 127.0.0.1", 
		"\t/sys root", 
		"", 
		"#", 
		"#/opt", 
		"/lib 127.0.0.1/32", 
		"/lib32 root (ugo) ",           // input after options, but skippable
		"/lib64 root (ugo)\t",          // input after options, but skippable
		};
	const size_t correct_count = sizeof(correct_exports) / sizeof(correct_exports[0]);
	size_t ignored_count = 0;

	for (size_t i = 0; i < correct_count; ++i)
	{
		std::string export_string = correct_exports[i];
		while (!export_string.empty() 
		&& (export_string[0] == ' ' || export_string[0] == '\t'))
		{
			export_string.erase(export_string.begin());
		}

		if (export_string.empty() 
		|| export_string[0] == '#')
		{
			++ignored_count;
		}
	}
		
	struct list *exports = NULL;
	std::ofstream stream;
	
	stream.open(exportsfile, std::ios_base::out | std::ios_base::trunc);
	CPPUNIT_ASSERT(stream.is_open());
	for (size_t i = 0; i < correct_count; ++i)
	{
		stream << correct_exports[i] << std::endl;
	}
	stream.close();
	CPPUNIT_ASSERT(!stream.fail());

	CPPUNIT_ASSERT(parse_exports(exportsfile, &exports, 0) == 0);

	struct list *item = exports;
	size_t count = 0;
	while (item != NULL)
	{
		++count;
		item = item->next;
	}
	
	CPPUNIT_ASSERT(count == (correct_count - ignored_count));

	release_exports(&exports);
	CPPUNIT_ASSERT(exports == NULL);
}

void TestExports::testWrongParsing()
{
	const char *wrong_exports[] = 
		{
		"/root",                                // no users
		"/proc (ro)",                           // no users
		"/boot 127.0.0.1 ( user=127.0.0.1 )",   // wrong user=
		"/lib root (ro, ugo)",                  // can't use ro with ugo
		"/opt root (user=root, ugo)",           // can't use user= with ugo
		"/var 127.0.0.1, root (ugo)",           // can't use ipaddr with ugo
		"/var 127.0.0.1, root (ugo) 127.0.0.2", // input after options
		};
	const size_t wrong_count = sizeof(wrong_exports) / sizeof(wrong_exports[0]);

	struct list *exports = NULL;
	std::ofstream stream;

	for (size_t i = 0; i < wrong_count; ++i)
	{
		stream.open(exportsfile, std::ios_base::out | std::ios_base::trunc);
		CPPUNIT_ASSERT(stream.is_open());
		stream << wrong_exports[i] << std::endl;
		stream.close();
		CPPUNIT_ASSERT(!stream.fail());

		CPPUNIT_ASSERT(parse_exports(exportsfile, &exports, 0) == 1); // ret should be line number starting from 1
		CPPUNIT_ASSERT(exports == NULL);
	}
}

void TestExports::testDupesParsing()
{
	const char *dup_exports[] = 
		{
		"/root root (ugo)", 
		"/root 127.0.0.1 (ro)", 
		};
	const size_t dup_count = sizeof(dup_exports) / sizeof(dup_exports[0]);

	struct list *exports = NULL;
	std::ofstream stream;
	
	stream.open(exportsfile, std::ios_base::out | std::ios_base::trunc);
	CPPUNIT_ASSERT(stream.is_open());
	for (size_t i = 0; i < dup_count; ++i)
	{
		stream << dup_exports[i] << std::endl;
	}
	stream.close();
	CPPUNIT_ASSERT(!stream.fail());

	CPPUNIT_ASSERT(parse_exports(exportsfile, &exports, 0) == 2); // ret should be line number starting from 1, second dup in this case
	CPPUNIT_ASSERT(exports == NULL);
}

void TestExports::testEmptyParsing()
{
	struct list *exports = NULL;
	std::ofstream stream;

	stream.open(exportsfile, std::ios_base::out | std::ios_base::trunc);
	CPPUNIT_ASSERT(stream.is_open());
	stream.close();
	CPPUNIT_ASSERT(!stream.fail());
	
	CPPUNIT_ASSERT(parse_exports(exportsfile, &exports, 0) == 0);
	CPPUNIT_ASSERT(exports == NULL);
}

