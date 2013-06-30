
#include <cppunit/extensions/HelperMacros.h>

#include "../src/config.h"
#include "../src/sockets.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>

class TestSockets : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TestSockets);
	CPPUNIT_TEST(testBlockUnblock);
	CPPUNIT_TEST_SUITE_END();

public:
	virtual void setUp();
	virtual void tearDown();

public:
	void testBlockUnblock();

private:
	int m_socket;
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestSockets);

void TestSockets::setUp()
{
	m_socket = socket(AF_UNIX, SOCK_STREAM, 0);
}

void TestSockets::tearDown()
{
	close(m_socket);
}

void TestSockets::testBlockUnblock()
{
	CPPUNIT_ASSERT((fcntl(m_socket, F_GETFL, NULL) & O_NONBLOCK) == 0);

	setup_socket_non_blocking(m_socket);
	CPPUNIT_ASSERT((fcntl(m_socket, F_GETFL, NULL) & O_NONBLOCK) == O_NONBLOCK);

	setup_socket_blocking(m_socket);
	CPPUNIT_ASSERT((fcntl(m_socket, F_GETFL, NULL) & O_NONBLOCK) == 0);
}
