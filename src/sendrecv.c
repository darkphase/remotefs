#include "sendrecv.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>

#include "config.h"
#include "command.h"
#include "alloc.h"
#include "inet.h"

int g_server_socket = -1;

#ifdef RFS_DEBUG
static unsigned long bytes_sent = 0;
static unsigned long bytes_received = 0;
static unsigned long receive_operations = 0;
static unsigned long send_operations = 0;
#endif

int rfs_connect(const char *ip, const unsigned port)
{
	int sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == -1)
	{
		return -1;
	}
	
	struct hostent *host_addr = gethostbyname(ip);
	if (host_addr == NULL)
	{
		return -1;
	}
	
	struct sockaddr_in server_addr = { 0 };
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	
	memcpy(&server_addr.sin_addr, host_addr->h_addr_list[0], sizeof(server_addr.sin_addr));

	if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
	{
		return -1;
	}
	
	g_server_socket = sock;
	
	return sock;
}

void rfs_disconnect(int sock)
{
	struct command cmd = { cmd_closeconnection, 0 };
	rfs_send_cmd(sock, &cmd);
	
	close(sock);
	
	mp_force_free();
}

unsigned rfs_send_cmd(const int sock, const struct command *cmd)
{
	struct command send_command = { 0 };
	send_command.command = cmd->command;
	send_command.data_len = htonl(cmd->data_len);
	
	DEBUG("%s", "sending "); dump_command(cmd);
	return rfs_send_data(sock, &send_command, sizeof(send_command));
}

unsigned rfs_send_answer(const int sock, const struct answer *ans)
{
	struct answer send_answer = { 0 };
	send_answer.command = ans->command;
	send_answer.data_len = htonl(ans->data_len);
	send_answer.ret = ans->ret;
	send_answer.ret_errno = ans->ret_errno;
	
	DEBUG("%s", "sending "); dump_answer(ans);
	return rfs_send_data(sock, &send_answer, sizeof(send_answer));
}

unsigned rfs_send_data(const int sock, const void *data, const unsigned data_len)
{
	DEBUG("sending data: %u bytes\n", data_len);
	unsigned size_sent = 0;
	
	while (size_sent < data_len)
	{
		int done = send(sock, data + size_sent, data_len - size_sent, 0);
		if (done == -1)
		{
			return -1;
		}
		size_sent += (unsigned)done;
	}

#ifdef RFS_DEBUG
	bytes_sent += size_sent;
	++send_operations;
#endif
	
	return size_sent;
}

unsigned rfs_receive_answer(const int sock, struct answer *ans)
{
	struct answer recv_answer = { 0 };

	unsigned ret = rfs_receive_data(sock, &recv_answer, sizeof(recv_answer));
	if (ret == sizeof(*ans))
	{
		ans->command = recv_answer.command;
		ans->data_len = ntohl(recv_answer.data_len);
		ans->ret = recv_answer.ret;
		ans->ret_errno = recv_answer.ret_errno;
	}
	return ret;
}

unsigned rfs_receive_cmd(const int sock, struct command *cmd)
{
	struct command recv_command = { 0 };
	
	unsigned ret = rfs_receive_data(sock, &recv_command, sizeof(recv_command));
	if (ret == sizeof(*cmd))
	{
		cmd->command = recv_command.command;
		cmd->data_len = ntohl(recv_command.data_len);
	}
	return ret;
}

unsigned rfs_receive_data(const int sock, void *data, const unsigned data_len)
{
	unsigned size_received = 0;
	while (size_received < data_len)
	{
		int done = recv(sock, data + size_received, data_len - size_received, 0);
		if (done == -1)
		{
			return -1;
		}
		size_received += (unsigned)done;
	}
	
#ifdef RFS_DEBUG
	bytes_received += size_received;
	++receive_operations;
#endif

	return size_received;
}

#ifdef RFS_DEBUG
void dump_sendrecv_stats()
{
	DEBUG("%s\n", "dumping transfer statistics:");
	DEBUG("total send operations: %lu\n", send_operations);
	DEBUG("total bytes sent: %lu\n", bytes_sent);
	DEBUG("total receive operations: %lu\n", receive_operations);
	DEBUG("total bytes received: %lu\n", bytes_received);
}
#endif

