
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <cppunit/extensions/HelperMacros.h>

#include "../src/config.h"
#include "../src/exports.h"
#include "../src/list.h"
#include "../src/utils.h"

class TestExports : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TestExports);
	CPPUNIT_TEST(testCorrectParsing);
	CPPUNIT_TEST(testWrongParsing);
	CPPUNIT_TEST(testDupesParsing);
	CPPUNIT_TEST(testEmptyParsing);
	CPPUNIT_TEST(testResolving);
	CPPUNIT_TEST(testResolvingLimits);
	CPPUNIT_TEST(testParamsParsing);
	CPPUNIT_TEST(testLineBreak);
	CPPUNIT_TEST_SUITE_END();

public:
	virtual void tearDown();
	void testCorrectParsing();
	void testWrongParsing();
	void testDupesParsing();
	void testEmptyParsing();
	void testResolving();
	void testResolvingLimits();
	void testParamsParsing();
	void testLineBreak();

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
		"/tmp root (ro)",
		"/sbin 127.0.0.1 ( ro)",
		"/home/alex root (user=root )",
		"/home 127.0.0.1,root( ro,user=root )",
		" /usr 127.0.0.1",
		"\t/sys root",
		"",
		"#",
		"#/opt",
		"/lib 127.0.0.1/32",
		"/lib32 root (ro) ",           // input after options, but skippable
		"/lib64 root (ro)\t",          // input after options, but skippable
		"/etc root@127.0.0.1",
		"/emul root@127.0.0.1/32",
		"/cdrom *",
#ifdef WITH_IPV6
		"/boot root@::1/128",
		"/dev root@::1/80",
#endif
#ifdef WITH_UGO
		"/home/test root (ugo)",
#endif
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

	struct rfs_list *exports = NULL;
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

	struct rfs_list *item = exports;
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
		"/var 127.0.0.1, root (ro) 127.0.0.2", // input after options
		"/etc root@127.0.0.1/33",               // too long prefix
#ifdef WITH_IPV6
		"/etc root@::1/129",                    // too long prefix
#endif
#ifdef WITH_UGO
		"/lib root (ro, ugo)",                  // can't use ro with ugo
		"/opt root (user=root, ugo)",           // can't use user= with ugo
		"/var 127.0.0.1, root (ugo)",           // can't use ipaddr with ugo
#endif
		};
	const size_t wrong_count = sizeof(wrong_exports) / sizeof(wrong_exports[0]);

	struct rfs_list *exports = NULL;
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
		"/root root",
		"/root 127.0.0.1 (ro)",
		};
	const size_t dup_count = sizeof(dup_exports) / sizeof(dup_exports[0]);

	struct rfs_list *exports = NULL;
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
	struct rfs_list *exports = NULL;
	std::ofstream stream;

	stream.open(exportsfile, std::ios_base::out | std::ios_base::trunc);
	CPPUNIT_ASSERT(stream.is_open());
	stream.close();
	CPPUNIT_ASSERT(!stream.fail());

	CPPUNIT_ASSERT(parse_exports(exportsfile, &exports, 0) == 0);
	CPPUNIT_ASSERT(exports == NULL);
}

void TestExports::testResolving()
{
	// need to avoid duplicate entries in exports paths
	const char *correct_exports[] =
		{
		"/tmp localhost", // parsed as username localhost
		"/bin @localhost", // parsed as network 127.0.0.1/32, ::1/128
		};

	const size_t correct_count = sizeof(correct_exports) / sizeof(correct_exports[0]);

	struct rfs_list *exports = NULL;
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

	size_t count = list_length(exports);
	CPPUNIT_ASSERT(count == correct_count);

	struct rfs_list *export_item = exports;
	while (export_item != NULL)
	{
		struct rfs_export *rec = (struct rfs_export *)export_item->data;

		CPPUNIT_ASSERT(rec->users != 0);

		struct user_rec *user = (struct user_rec *)rec->users->data;

		CPPUNIT_ASSERT(export_item == exports
			? user->network == NULL
			: user->network != NULL && is_ipaddr(user->network) != 0);

		if (user->network != NULL)
		{
			CPPUNIT_ASSERT(user->prefix_len > 0);
		}

		export_item = export_item->next;
	}

	release_exports(&exports);
	CPPUNIT_ASSERT(exports == NULL);
}

void TestExports::testResolvingLimits()
{
	const char *wrong_exports[] =
		{
		"/tmp @locahost/24", // security issue
		};
	const size_t wrong_count = sizeof(wrong_exports) / sizeof(wrong_exports[0]);

	struct rfs_list *exports = NULL;
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

void TestExports::testParamsParsing()
{
	// need to avoid duplicate entries in exports paths
	const char *correct_export = "/var 127.0.0.1/24, 127.0.1.1/16, 127.0.1.2, localhost, root@127.0.0.1, @localhost";

	std::ofstream stream;
	stream.open(exportsfile, std::ios_base::out | std::ios_base::trunc);
	CPPUNIT_ASSERT(stream.is_open());
	stream << correct_export << std::endl;
	stream.close();
	CPPUNIT_ASSERT(!stream.fail());

	struct rfs_list *exports = NULL;
	CPPUNIT_ASSERT(parse_exports(exportsfile, &exports, 0) == 0);

	struct rfs_export *exp = (struct rfs_export *)exports->data;
	struct rfs_list *user_rec = exp->users;

	unsigned position = 0;
	while (user_rec != NULL)
	{
		++position;

		struct user_rec *user = (struct user_rec *)user_rec->data;

		switch (position)
		{
		case 1: // 127.0.0.1/24
			CPPUNIT_ASSERT(user->username == NULL);
			CPPUNIT_ASSERT(user->network != NULL);
			CPPUNIT_ASSERT(strcmp(user->network, "127.0.0.1") == 0);
			CPPUNIT_ASSERT(user->prefix_len == 24);
			break;

		case 2: // 127.0.1.1/16
			CPPUNIT_ASSERT(user->username == NULL);
			CPPUNIT_ASSERT(user->network != NULL);
			CPPUNIT_ASSERT(strcmp(user->network, "127.0.1.1") == 0);
			CPPUNIT_ASSERT(user->prefix_len == 16);
			break;

		case 3: // 127.0.1.2
			CPPUNIT_ASSERT(user->username == NULL);
			CPPUNIT_ASSERT(user->network != NULL);
			CPPUNIT_ASSERT(strcmp(user->network, "127.0.1.2") == 0);
			CPPUNIT_ASSERT(user->prefix_len == 32);
			break;

		case 4: // localhost (not resolved)
			CPPUNIT_ASSERT(user->network == NULL);
			break;

		case 5: // root@127.0.0.1
			CPPUNIT_ASSERT(user->network != NULL);
			CPPUNIT_ASSERT(strcmp(user->network, "127.0.0.1") == 0);
			CPPUNIT_ASSERT(user->username != NULL);
			CPPUNIT_ASSERT(strcmp(user->username, "root") == 0);
			CPPUNIT_ASSERT(user->prefix_len == 32);
			break;

		case 6: // @localhost, some numbers are reserved for hostname expansion
		case 7:
		case 8:
			CPPUNIT_ASSERT(user->username == NULL);
			CPPUNIT_ASSERT(user->network != NULL);
			if (strchr(user->network, ':') != NULL) // IPv6
			{
				CPPUNIT_ASSERT(user->prefix_len == 128);
			}
			else // IPv4
			{
				CPPUNIT_ASSERT(user->prefix_len == 32);
			}
			break;
		}

		user_rec = user_rec->next;
	}

	release_exports(&exports);
	CPPUNIT_ASSERT(exports == NULL);
}

void TestExports::testLineBreak()
{
	const std::string export_line("/tmp 127.0.0.1");

	struct rfs_list *exports = NULL;
	std::ofstream stream;

	stream.open(exportsfile, std::ios_base::out | std::ios_base::trunc);
	CPPUNIT_ASSERT(stream.is_open());
	stream << export_line << std::endl; // first case: with line break at the end
	stream.close();
	CPPUNIT_ASSERT(!stream.fail());

	CPPUNIT_ASSERT(parse_exports(exportsfile, &exports, 0) == 0);
	CPPUNIT_ASSERT(exports != NULL);

	release_exports(&exports);
	CPPUNIT_ASSERT(exports == NULL);

	stream.open(exportsfile, std::ios_base::out | std::ios_base::trunc);
	CPPUNIT_ASSERT(stream.is_open());
	stream << export_line; // second case: no line break at the end
	stream.close();
	CPPUNIT_ASSERT(!stream.fail());

	CPPUNIT_ASSERT(parse_exports(exportsfile, &exports, 0) == 0);
	CPPUNIT_ASSERT(exports != NULL);
}
