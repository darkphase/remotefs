
#include <string.h>
#include <unistd.h>
#include <cppunit/extensions/HelperMacros.h>

#include "../src/config.h"
#include "../src/passwd.h"

class TestPasswd : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TestPasswd);
	CPPUNIT_TEST(testSaveLoad);
	CPPUNIT_TEST_SUITE_END();

public:
	virtual void tearDown();
	void testSaveLoad();

protected:
	static const char *pwdfile;
};

const char* TestPasswd::pwdfile = "./rfs-passwd";

CPPUNIT_TEST_SUITE_REGISTRATION(TestPasswd);

void TestPasswd::tearDown()
{
	unlink(pwdfile);
}

void TestPasswd::testSaveLoad()
{
	const char *user1 = "cppunit1";
	const char *user2 = "cppunit2";
	const char *password = "cppunit";
	struct list *auths = NULL;

	CPPUNIT_ASSERT(load_passwords(pwdfile, &auths) == 0);
	CPPUNIT_ASSERT(add_or_replace_auth(&auths, user1, password) == 0);
	CPPUNIT_ASSERT(add_or_replace_auth(&auths, user2, password) == 0);
	CPPUNIT_ASSERT(save_passwords(pwdfile, auths) == 0);
	release_passwords(&auths);
	CPPUNIT_ASSERT(auths == NULL);
	
	CPPUNIT_ASSERT(load_passwords(pwdfile, &auths) == 0);
	CPPUNIT_ASSERT(strcmp(get_auth_password(auths, user1), password) == 0);
	CPPUNIT_ASSERT(strcmp(get_auth_password(auths, user2), password) == 0);
	CPPUNIT_ASSERT(delete_auth(&auths, user1) == 0);
	CPPUNIT_ASSERT(save_passwords(pwdfile, auths) == 0);
	release_passwords(&auths);
	CPPUNIT_ASSERT(auths == NULL);
	
	CPPUNIT_ASSERT(load_passwords(pwdfile, &auths) == 0);
	CPPUNIT_ASSERT(get_auth_password(auths, user1) == NULL);
	CPPUNIT_ASSERT(strcmp(get_auth_password(auths, user2), password) == 0);
	release_passwords(&auths);
	CPPUNIT_ASSERT(auths == NULL);
}

