#include "sendrecv.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>

#include "config.h"
#include "command.h"
#include "alloc.h"
#include "inet.h"

int g_server_socket = -1;

#ifdef RFS_DEBUG
static size_t bytes_sent = 0;
static size_t bytes_received = 0;
static size_t receive_operations = 0;
static size_t send_operations = 0;
#endif

int connection_lost = 0;

int rfs_is_connection_lost()
{
	return connection_lost;}

void rfs_set_connection_restored()
{
	connection_lost = 0;}

int rfs_connect(const char *ip, const unsigned port)
{
	errno = 0;
	int sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == -1)
	{
		return -errno;
	}
	
	errno = 0;
	struct hostent *host_addr = gethostbyname(ip);
	if (host_addr == NULL)
	{
		return -errno;
	}
	
	struct sockaddr_in server_addr = { 0 };
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	
	memcpy(&server_addr.sin_addr, host_addr->h_addr_list[0], sizeof(server_addr.sin_addr));

	errno = 0;
	if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
	{
		return -errno;
	}
	
	g_server_socket = sock;
	
	return sock;
}

void rfs_disconnect(int sock, int gently)
{
	if (gently != 0)
	{
		struct command cmd = { cmd_closeconnection, 0 };
		rfs_send_cmd(sock, &cmd);
	}
	
	close(sock);
	shutdown(sock, SHUT_RDWR);
	
	mp_force_free();
}

size_t rfs_send_cmd(const int sock, const struct command *cmd)
{
	struct command send_command = { 0 };
	send_command.command = htonl(cmd->command);
	send_command.data_len = htonl(cmd->data_len);
	
	DEBUG("%s", "sending "); dump_command(cmd);
	size_t ret = rfs_send_data(sock, &send_command, sizeof(send_command));
	if (ret == 0)
	{
		connection_lost = 1;
	}
	return ret;
}

size_t rfs_send_answer(const int sock, const struct answer *ans)
{
	struct answer send_answer = { 0 };
	send_answer.command = htonl(ans->command);
	send_answer.data_len = htonl(ans->data_len);
	send_answer.ret = htonl(ans->ret);
	send_answer.ret_errno = htons(ans->ret_errno);
	
	DEBUG("%s", "sending "); dump_answer(ans);
	size_t ret = rfs_send_data(sock, &send_answer, sizeof(send_answer));
	if (ret == 0)
	{
		connection_lost = 1;
	}
	return ret;
}

size_t rfs_send_data(const int sock, const void *data, const size_t data_len)
{
	DEBUG("sending data: %u bytes\n", data_len);
	size_t size_sent = 0;
	
	while (size_sent < data_len)
	{
		int done = send(sock, data + size_sent, data_len - size_sent, 0);
		if (done < 1)
		{
			connection_lost = 1;
			return -1;
		}
		size_sent += (size_t)done;
	}

#ifdef RFS_DEBUG
	bytes_sent += size_sent;
	++send_operations;
#endif
	
	return size_sent;
}

size_t rfs_receive_answer(const int sock, struct answer *ans)
{
	struct answer recv_answer = { 0 };

	size_t ret = rfs_receive_data(sock, &recv_answer, sizeof(recv_answer));
	if (ret == sizeof(*ans))
	{
		ans->command = ntohl(recv_answer.command);
		ans->data_len = ntohl(recv_answer.data_len);
		ans->ret = ntohl(recv_answer.ret);
		ans->ret_errno = ntohs(recv_answer.ret_errno);
	}
	else
	{
		connection_lost = 1;
	}
	
	DEBUG("%s", "received "); dump_answer(ans);
	
	return ret;
}

size_t rfs_receive_cmd(const int sock, struct command *cmd)
{
	struct command recv_command = { 0 };
	
	size_t ret = rfs_receive_data(sock, &recv_command, sizeof(recv_command));
	if (ret == sizeof(*cmd))
	{
		cmd->command = ntohl(recv_command.command);
		cmd->data_len = ntohl(recv_command.data_len);
	}
	else
	{
		connection_lost = 1;
	}
	DEBUG("%s", "received "); dump_command(cmd);
	return ret;
}

size_t rfs_receive_data(const int sock, void *data, const size_t data_len)
{
	size_t size_received = 0;
	while (size_received < data_len)
	{
		int done = recv(sock, data + size_received, data_len - size_received, 0);
		if (done < 1)
		{
			connection_lost = 1;
			return -1;
		}
		size_received += (size_t)done;
	}
	
#ifdef RFS_DEBUG
	bytes_received += size_received;
	++receive_operations;
#endif

	return size_received;
}

size_t rfs_ignore_incoming_data(const int sock, const size_t data_len)
{
	size_t size_ignored = 0;
	char buffer[4096] = { 0 };
	
	while (size_ignored < data_len)
	{
		int done = recv(sock, buffer, data_len - size_ignored > sizeof(buffer) ? sizeof(buffer) : data_len - size_ignored, 0);
		if (done < 1)
		{
			connection_lost = 1;
			return -1;
		}
		size_ignored += (size_t)done;
	}
	
	return size_ignored;
}

#ifdef RFS_DEBUG
void dump_sendrecv_stats()
{
	DEBUG("%s\n", "dumping transfer statistics:");
	DEBUG("total send operations: %lu\n", (unsigned long)send_operations);
	DEBUG("total bytes sent: %lu\n", (unsigned long)bytes_sent);
	DEBUG("total receive operations: %lu\n", (unsigned long)receive_operations);
	DEBUG("total bytes received: %lu\n", (unsigned long)bytes_received);
}
#endif

