
#include <cppunit/extensions/HelperMacros.h>
#include <limits.h>

#include "../src/config.h"
#include "../src/sendrecv_client.h"
#include "../src/sendrecv_server.h"

class TestSendrecv : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TestSendrecv);
	//CPPUNIT_TEST(testHTON);
	CPPUNIT_TEST(testQueue);
	CPPUNIT_TEST(testMaxIOVecs);
	CPPUNIT_TEST_SUITE_END();

public:
	void testHTON();
	void testQueue();
	void testMaxIOVecs();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestSendrecv);

/* FIXME: ntoh_* still need some coverage
void TestSendrecv::testHTON()
{
	struct rfs_command cmd = { cmd_first, 1 }, cmd_copy = cmd;
	struct rfs_answer ans = { cmd_first, 1, 2, 3 }, ans_copy = ans;

	CPPUNIT_ASSERT(memcmp(ntoh_cmd(hton_cmd(&cmd)), &cmd_copy, sizeof(cmd_copy)) == 0);
	CPPUNIT_ASSERT(memcmp(ntoh_ans(hton_ans(&ans)), &ans_copy, sizeof(ans_copy)) == 0);
}
*/

void TestSendrecv::testQueue()
{
	struct rfs_command cmd = { cmd_first, 1 }, cmd_copy = cmd;
	struct rfs_answer ans = { cmd_first, 1, 2, 3 }, ans_copy = ans;
	uint16_t u16 = 1, u16_copy = u16;
	uint32_t u32 = 2, u32_copy = u32;
	uint64_t u64 = 3, u64_copy = u64;
#define DATA "abc"
	char data[] = DATA;
	const char data_copy[] = DATA;
#undef DATA

	send_token_t token = { 0 };
	CPPUNIT_ASSERT(
	queue_cmd(&cmd,
	queue_ans(&ans,
	queue_16(&u16,
	queue_32(&u32,
	queue_64(&u64,
	queue_data(data, sizeof(data), &token
	)))))) == &token);

	CPPUNIT_ASSERT(token.count == 6);
	CPPUNIT_ASSERT(token.iov[0].iov_base = data);
	CPPUNIT_ASSERT(token.iov[0].iov_len = sizeof(data));
	CPPUNIT_ASSERT(token.iov[1].iov_base = &u64);
	CPPUNIT_ASSERT(token.iov[1].iov_len = sizeof(u64));
	CPPUNIT_ASSERT(token.iov[2].iov_base = &u32);
	CPPUNIT_ASSERT(token.iov[2].iov_len = sizeof(u32));
	CPPUNIT_ASSERT(token.iov[3].iov_base = &u64);
	CPPUNIT_ASSERT(token.iov[3].iov_len = sizeof(u64));
	CPPUNIT_ASSERT(token.iov[4].iov_base = &ans);
	CPPUNIT_ASSERT(token.iov[4].iov_len = sizeof(ans));
	CPPUNIT_ASSERT(token.iov[5].iov_base = &cmd);
	CPPUNIT_ASSERT(token.iov[5].iov_len = sizeof(cmd));

	CPPUNIT_ASSERT(memcmp(&cmd, &cmd_copy, sizeof(cmd)) == 0);
	CPPUNIT_ASSERT(memcmp(&ans, &ans_copy, sizeof(ans)) == 0);
	CPPUNIT_ASSERT(memcmp(data, data_copy, sizeof(data)) == 0);
	CPPUNIT_ASSERT(u64 == u64_copy); // check if queued data isn't hton'ed (since 0.14)
	CPPUNIT_ASSERT(u32 == u32_copy);
	CPPUNIT_ASSERT(u16 == u16_copy);
}

void TestSendrecv::testMaxIOVecs()
{
	char buffer[256] = { 0 };

	send_token_t token = { IOV_MAX - 1 };
	CPPUNIT_ASSERT(queue_data(buffer, sizeof(buffer), &token) == &token);

	send_token_t token1 = { IOV_MAX };
	CPPUNIT_ASSERT(queue_data(buffer, sizeof(buffer), &token1) == NULL);

	send_token_t token2 = { 0 };
	for (size_t i = 0; i < IOV_MAX; ++i)
	{
		CPPUNIT_ASSERT(queue_data(buffer, sizeof(buffer), &token2) == &token2);
	}
	CPPUNIT_ASSERT(token2.count == IOV_MAX); // IOV_MAX should be fine

	send_token_t token3 = { 0 };
	for (size_t i = 0; i < IOV_MAX + 1; ++i)
	{
		CPPUNIT_ASSERT(queue_data(buffer, sizeof(buffer), &token3) == (i < IOV_MAX ? &token3 : NULL));
	}
	CPPUNIT_ASSERT(token3.count == IOV_MAX); // max queued buffers
}

