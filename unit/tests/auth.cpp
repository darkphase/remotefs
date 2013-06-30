
#include <stdlib.h>
#include <string.h>
#include <cppunit/extensions/HelperMacros.h>

#include "../src/config.h"
#include "../src/crypt.h"
#include "../src/auth.h"
#include "../src/exports.h"
#include "../src/list.h"
#include "../src/passwd.h"
#include "../src/instance_server.h"

class TestAuth : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TestAuth);
	CPPUNIT_TEST(testSaltGeneration);
	CPPUNIT_TEST(testCheckPermissions);
#ifdef WITH_UGO
	CPPUNIT_TEST(testCheckUGOPermissions);
#endif
#ifdef WITH_IPV6
	CPPUNIT_TEST(testIPVersionFiltering);
#endif
	CPPUNIT_TEST_SUITE_END();

public:
	virtual void setUp();
	virtual void tearDown();

	void testSaltGeneration();
	void testCheckPermissions();
#ifdef WITH_UGO
	void testCheckUGOPermissions();
#endif
#ifdef WITH_IPV6
	void testIPVersionFiltering();
#endif

protected:
	struct rfsd_instance m_instance;
	
	static const char* exportsfile;
};

const char* TestAuth::exportsfile = "./rfs-exports";

CPPUNIT_TEST_SUITE_REGISTRATION(TestAuth);

void TestAuth::setUp()
{
	init_rfsd_instance(&m_instance);

	// prepare in-memory passwd database
	const char *valid_user[2] = { "root", "root" };
	CPPUNIT_ASSERT(add_or_replace_auth(&m_instance.passwd.auths, valid_user[0], valid_user[1]) == 0);

	// prepare auth in server's instance
	m_instance.server.auth_user = strdup(valid_user[0]);
	CPPUNIT_ASSERT(generate_salt(m_instance.server.auth_salt, sizeof(m_instance.server.auth_salt)) == 0);
	m_instance.server.auth_passwd = passwd_hash(get_auth_password(m_instance.passwd.auths, valid_user[0]), m_instance.server.auth_salt);
	CPPUNIT_ASSERT(m_instance.server.auth_passwd != NULL);
}

void TestAuth::tearDown()
{
	release_exports(&m_instance.exports.list);
	CPPUNIT_ASSERT(m_instance.exports.list == NULL);

	release_passwords(&m_instance.passwd.auths);
	CPPUNIT_ASSERT(m_instance.passwd.auths == NULL);

	unlink(exportsfile);

	release_rfsd_instance(&m_instance);
}

void TestAuth::testSaltGeneration()
{
	char salt[32 + 1] = { 0 };
	size_t max_size = sizeof(salt);

	CPPUNIT_ASSERT(1 < strlen(EMPTY_SALT)); // condition for the next test
	CPPUNIT_ASSERT(generate_salt(salt, 1) == -1); // shouldn't fill buffer if it's lesst than EMPTY_SALT

	CPPUNIT_ASSERT(max_size > strlen(EMPTY_SALT)); // condition for the next tests
	CPPUNIT_ASSERT(generate_salt(salt, strlen(EMPTY_SALT) + 1) == 0);
	CPPUNIT_ASSERT(strcmp(salt, EMPTY_SALT) == 0);

	// since generate_salt() is random, we're doing reasonable set of checks for valid salt
	for (int i = 0; i < 100; ++i)
	{
		memset(salt, 0, sizeof(salt));
		CPPUNIT_ASSERT(generate_salt(salt, max_size) == 0);
		CPPUNIT_ASSERT(strlen(salt) == max_size - 1);
		CPPUNIT_ASSERT(strstr(salt, EMPTY_SALT) == salt);
		for (size_t j = strlen(EMPTY_SALT); j < strlen(salt); ++j)
		{
			CPPUNIT_ASSERT(isalnum(salt[j]) != 0 || salt[j] == '.' || salt[j] == '/');
		}
	}
}

void TestAuth::testCheckPermissions()
{
	// prepare exports
	const char *correct_exports[] = 
		{
		"/dev *",                      // 0
		"/var 127.0.0.1",              // 1
		"/etc root",                   // 2
		"/mnt 127.0.0.1, root",        // 3
		"/boot 127.0.0.0/24",          // 4
		"/tmp root@127.0.0.1/24"       // 5
		"/tmp *@127.0.0.1/24"          // 6
#ifdef WITH_IPV6
		"/home ::/0",                  // 7
#endif
		};
	const size_t correct_count = sizeof(correct_exports) / sizeof(correct_exports[0]);
	
	std::ofstream stream;	
	stream.open(exportsfile, std::ios_base::out | std::ios_base::trunc);
	CPPUNIT_ASSERT(stream.is_open());

	for (size_t i = 0; i < correct_count; ++i)
	{
		stream << correct_exports[i] << std::endl;
	}
	stream.close();
	CPPUNIT_ASSERT(!stream.fail());
	
	CPPUNIT_ASSERT(parse_exports(exportsfile, &m_instance.exports.list, 0) == 0);

	const struct list *export_rec = m_instance.exports.list;
	unsigned export_no = 0;
	while (export_rec != NULL)
	{
		const struct rfs_export *info = static_cast<const struct rfs_export *>(export_rec->data);

		switch (export_no)
		{
			// /dev *
			case 0:
#ifdef WITH_IPV6
				CPPUNIT_ASSERT(check_permissions(&m_instance, info, "::1") == 0);
				CPPUNIT_ASSERT(check_permissions(&m_instance, info, "::ffff:127.0.0.1") == 0);
				CPPUNIT_ASSERT(check_permissions(&m_instance, info, "::ffff:192.168.0.1") == 0);
				CPPUNIT_ASSERT(check_permissions(&m_instance, info, "::ffff:8.8.8.8") == 0);
#endif
				CPPUNIT_ASSERT(check_permissions(&m_instance, info, "127.0.0.1") == 0);
				CPPUNIT_ASSERT(check_permissions(&m_instance, info, "192.168.0.1") == 0);
				CPPUNIT_ASSERT(check_permissions(&m_instance, info, "8.8.8.8") == 0);
				
				CPPUNIT_ASSERT(check_permissions(&m_instance, info, "root") == 0);
				break;

			// /var root, 127.0.0.1
			case 1:
				CPPUNIT_ASSERT(check_permissions(&m_instance, info, "127.0.0.0") != 0);
				CPPUNIT_ASSERT(check_permissions(&m_instance, info, "127.0.0.2") != 0);
				CPPUNIT_ASSERT(check_permissions(&m_instance, info, "127.0.0.1") == 0);
#ifdef WITH_IPV6
				CPPUNIT_ASSERT(check_permissions(&m_instance, info, "::ffff:127.0.0.2") != 0);
				CPPUNIT_ASSERT(check_permissions(&m_instance, info, "::ffff:127.0.0.1") == 0);
#endif
				break;

			// /etc root
			case 2:
				CPPUNIT_ASSERT(check_permissions(&m_instance, info, "127.0.0.1") == 0);
				
				{
				char *old_user = m_instance.server.auth_user;
				m_instance.server.auth_user = strdup("notroot");
				CPPUNIT_ASSERT(check_permissions(&m_instance, info, "127.0.0.1") != 0);
				free(m_instance.server.auth_user);
				m_instance.server.auth_user = old_user;
				}
				break;

			// /mnt 127.0.0.1, root
			case 3:
				CPPUNIT_ASSERT(check_permissions(&m_instance, info, "127.0.0.1") == 0);

				{
				char *old_user = m_instance.server.auth_user;
				m_instance.server.auth_user = strdup("notroot");
				CPPUNIT_ASSERT(check_permissions(&m_instance, info, "127.0.0.1") == 0); // user is different, but still can pass by netmask
				free(m_instance.server.auth_user);
				m_instance.server.auth_user = old_user;
				}
				break;

			// /boot 127.0.0.0/24
			case 4:
				CPPUNIT_ASSERT(check_permissions(&m_instance, info, "127.0.0.0") == 0);
				CPPUNIT_ASSERT(check_permissions(&m_instance, info, "127.0.0.1") == 0);
				CPPUNIT_ASSERT(check_permissions(&m_instance, info, "127.0.0.2") == 0);
				CPPUNIT_ASSERT(check_permissions(&m_instance, info, "127.0.1.1") != 0);
				break;

			// /tmp root@127.0.0.1/24
			case 5:
				CPPUNIT_ASSERT(check_permissions(&m_instance, info, "127.0.0.1") == 0);
				CPPUNIT_ASSERT(check_permissions(&m_instance, info, "127.0.1.1") != 0);

				{
				char *old_user = m_instance.server.auth_user;
				m_instance.server.auth_user = strdup("notroot");
				CPPUNIT_ASSERT(check_permissions(&m_instance, info, "127.0.0.1") != 0);
				free(m_instance.server.auth_user);
				m_instance.server.auth_user = old_user;
				}
				break;

			// /tmp *@127.0.0.1/24
			case 6:
				CPPUNIT_ASSERT(check_permissions(&m_instance, info, "127.0.0.1") == 0);
				CPPUNIT_ASSERT(check_permissions(&m_instance, info, "127.0.1.1") != 0);
				break;

#ifdef WITH_IPV6
			// /home ::/0"
			case 7:
				CPPUNIT_ASSERT(check_permissions(&m_instance, info, "::ffff:127.0.0.1") == 0);
				CPPUNIT_ASSERT(check_permissions(&m_instance, info, "::1") == 0);
				break;
#endif
			default:
				CPPUNIT_ASSERT(false);
		}

		++export_no;
		export_rec = export_rec->next;
	}
}

#ifdef WITH_UGO
void TestAuth::testCheckUGOPermissions()
{
	// prepare exports
	const char *correct_exports[] = 
		{
		"/dev * (ugo)",                // 0
		};
	const size_t correct_count = sizeof(correct_exports) / sizeof(correct_exports[0]);
	
	std::ofstream stream;	
	stream.open(exportsfile, std::ios_base::out | std::ios_base::trunc);
	CPPUNIT_ASSERT(stream.is_open());

	for (size_t i = 0; i < correct_count; ++i)
	{
		stream << correct_exports[i] << std::endl;
	}
	stream.close();
	CPPUNIT_ASSERT(!stream.fail());
	
	CPPUNIT_ASSERT(parse_exports(exportsfile, &m_instance.exports.list, 0) == 0);

	const struct list *export_rec = m_instance.exports.list;
	unsigned export_no = 0;
	while (export_rec != NULL)
	{
		const struct rfs_export *info = static_cast<const struct rfs_export *>(export_rec->data);

		switch (export_no)
		{
			// /dev * (ugo)
			case 0:
				CPPUNIT_ASSERT(check_permissions(&m_instance, info, "") == 0);
				
				// break server's auth to check if login will fail
				char *auth_user = m_instance.server.auth_user;
				m_instance.server.auth_user = NULL;
				CPPUNIT_ASSERT(check_permissions(&m_instance, info, "") != 0);
				m_instance.server.auth_user = auth_user;
				break;
		}

		++export_no;
		export_rec = export_rec->next;
	}
}
#endif /* WITH_UGO */

#ifdef WITH_IPV6
void TestAuth::testIPVersionFiltering()
{
	// prepare exports
	const char *correct_exports[] = 
		{
		"/home ::/0",                  // 0
		"/dev 0.0.0.0/0" ,             // 1
		};
	const size_t correct_count = sizeof(correct_exports) / sizeof(correct_exports[0]);
	
	std::ofstream stream;	
	stream.open(exportsfile, std::ios_base::out | std::ios_base::trunc);
	CPPUNIT_ASSERT(stream.is_open());

	for (size_t i = 0; i < correct_count; ++i)
	{
		stream << correct_exports[i] << std::endl;
	}
	stream.close();
	CPPUNIT_ASSERT(!stream.fail());
	
	CPPUNIT_ASSERT(parse_exports(exportsfile, &m_instance.exports.list, 0) == 0);

	const struct list *export_rec = m_instance.exports.list;
	unsigned export_no = 0;
	while (export_rec != NULL)
	{
		const struct rfs_export *info = static_cast<const struct rfs_export *>(export_rec->data);

		switch (export_no)
		{
			// /home ::/0
			case 0:
				CPPUNIT_ASSERT(check_permissions(&m_instance, info, "::1") == 0);
				CPPUNIT_ASSERT(check_permissions(&m_instance, info, "127.0.0.1") != 0);
				break;

			// /dev 0.0.0.0/0
			case 1:
				CPPUNIT_ASSERT(check_permissions(&m_instance, info, "::1") != 0);
				CPPUNIT_ASSERT(check_permissions(&m_instance, info, "127.0.0.1") == 0);
				break;
		}

		++export_no;
		export_rec = export_rec->next;
	}
}
#endif /* WITH_IPV6 */

