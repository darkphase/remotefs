
#include <cppunit/extensions/HelperMacros.h>

#include "config.h"
#include "sendrecv.h"

class TestSendrecv : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TestSendrecv);
	CPPUNIT_TEST(testHTON);
	CPPUNIT_TEST(testQueue);
	CPPUNIT_TEST(testMaxIOVecs);
	CPPUNIT_TEST_SUITE_END();

public:
	void testHTON();
	void testQueue();
	void testMaxIOVecs();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestSendrecv);

void TestSendrecv::testHTON()
{
	struct command cmd = { cmd_first, 1 }, cmd_copy = cmd;
	struct answer ans = { cmd_first, 1, 2, 3 }, ans_copy = ans;

	CPPUNIT_ASSERT(memcmp(ntoh_cmd(hton_cmd(&cmd)), &cmd_copy, sizeof(cmd_copy)) == 0);
	CPPUNIT_ASSERT(memcmp(ntoh_ans(hton_ans(&ans)), &ans_copy, sizeof(ans_copy)) == 0);
}

void TestSendrecv::testQueue()
{
	struct command cmd = { cmd_first, 1 }, cmd_copy = cmd;
	struct answer ans = { cmd_first, 1, 2, 3 }, ans_copy = ans;
	uint16_t u16 = 1, u16_copy = u16;
	uint32_t u32 = 2, u32_copy = u32;
	uint64_t u64 = 3, u64_copy = u64;
#define DATA "abc"
	char data[] = DATA;
	const char data_copy[] = DATA;
#undef DATA

	send_token_t token = { 0, {{ 0 }} };
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

	CPPUNIT_ASSERT(memcmp(data, data_copy, sizeof(data)) == 0);
	CPPUNIT_ASSERT(memcmp(&cmd, hton_cmd(&cmd_copy), sizeof(cmd)) == 0);
	CPPUNIT_ASSERT(memcmp(&ans, hton_ans(&ans_copy), sizeof(ans)) == 0);
	CPPUNIT_ASSERT(u64 == htonll(u64_copy));
	CPPUNIT_ASSERT(u32 == htonl(u32_copy));
	CPPUNIT_ASSERT(u16 == htons(u16_copy));
}

void TestSendrecv::testMaxIOVecs()
{
	send_token_t token0 = { 15, {{ 0 }} };
	CPPUNIT_ASSERT(queue_data(NULL, 0, &token0) == &token0);

	send_token_t token1 = { 16, {{ 0 }} };
	CPPUNIT_ASSERT(queue_data(NULL, 0, &token1) == NULL);
	
	send_token_t token2 = { 0, {{ 0 }} };
	for (size_t i = 0; i < 16; ++i)
	{
		CPPUNIT_ASSERT(queue_data(NULL, 0, &token2) == &token2);
	}
	CPPUNIT_ASSERT(token2.count == 16); // 16 should be fine

	send_token_t token3 = { 0, {{ 0 }} };
	for (size_t i = 0; i < 17; ++i)
	{
		CPPUNIT_ASSERT(queue_data(NULL, 0, &token3) == (i < 16 ? &token3 : NULL));
	}
	CPPUNIT_ASSERT(token3.count == 16); // max queued buffers
}

