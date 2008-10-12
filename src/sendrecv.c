/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <sys/uio.h>

#include "config.h"
#include "sendrecv.h"
#include "command.h"
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
	return connection_lost;
}

void rfs_set_connection_restored()
{
	connection_lost = 0;
}

int rfs_connect(const char *ip, const unsigned port)
{
#ifndef WITH_IPV6
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
#else
	struct sockaddr_in *sa4;
	struct sockaddr_in6 *sa6;
	struct addrinfo *res;
	struct addrinfo hints;
	int    result;

	/* resolve name or address */
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family    = AF_UNSPEC;   /* Allow IPv4 or IPv6 */
	hints.ai_socktype  = SOCK_STREAM; 
	hints.ai_flags     = AI_ADDRCONFIG;

	if ((result=getaddrinfo(ip, NULL, &hints, &res)) != 0)
	{
		ERROR("Can't resolve address for %s : %s\n",ip,gai_strerror(result));
		return -errno;
	}

	sa4 = (struct sockaddr_in*)res->ai_addr;
	sa6 = (struct sockaddr_in6*)res->ai_addr;


	int sock = socket(res->ai_family, SOCK_STREAM, 0);
	if (sock == -1)
	{
		return -errno;
	}

	sa4 = (struct sockaddr_in*)res->ai_addr;
	sa6 = (struct sockaddr_in6*)res->ai_addr;
	if (res->ai_family == AF_INET)
	{
		sa4->sin_port = htons(port);
	}
	else
	{
		sa6->sin6_port = htons(port);
	}

	if (connect(sock, (struct sockaddr *)res->ai_addr, res->ai_addrlen) == -1)
	{
		freeaddrinfo(res);
		return -errno;
	}
#endif
	g_server_socket = sock;
	
	return sock;
}

#ifdef RFS_DEBUG
static void dump_iov(struct iovec *iov, unsigned count)
{
	DEBUG("dumping %u io vectors:\n", count);
	unsigned i = 0; for (i = 0; i < count; ++i)
	{
		DEBUG("%u: base: %p, len: %u\n", i, iov[i].iov_base, iov[i].iov_len);
	}
}
#endif

static int fix_iov(struct iovec *iov, unsigned count, size_t size_left)
{
	size_t overall_size = 0;
	
	{
	int i = 0; for (i = 0; i < count; ++i)
	{
		overall_size += iov[i].iov_len;
	}
	}
	
	if (size_left == overall_size)
	{
		return count;
	}
	
	size_t diff = overall_size - size_left;
	
#ifdef RFS_DEBUG
	DEBUG("size left: %u, diff: %u\n", size_left, diff);
	dump_iov(iov, count);
#endif
	
	int ret = count;
	int i = 0; for (i = 0; i < count; )
	{
		if (iov[i].iov_len <= diff)
		{
			diff -= iov[i].iov_len;
			if (i < count)
			{
				iov[i].iov_len = iov[i + 1].iov_len;
				iov[i].iov_base = iov[i + 1].iov_base;
				--ret;
			}
			else
			{
				iov[i].iov_len = 0;
				--ret;
				break;
			}
		}
		else if (iov[i].iov_len > diff)
		{
			iov[i].iov_base += diff;
			iov[i].iov_len -= diff;
			break;
		}
	}
	
#ifdef RFS_DEBUG
	dump_iov(iov, ret);
#endif
	
	return ret;
}

static ssize_t rfs_writev(const int sock, struct iovec *iov, unsigned count)
{
	size_t overall_size = 0;
	int i = 0; for (i = 0; i < count; ++i)
	{
		overall_size += iov[i].iov_len;
	}
	
	DEBUG("sending data: %u bytes\n", overall_size);
	
	ssize_t size_sent = 0;
	while (size_sent < overall_size)
	{
		count = fix_iov(iov, count, overall_size - size_sent);
		
		errno = 0;
		int done = writev(sock, iov, count);
		if (done < 0)
		{
			if (errno == EAGAIN || errno == EINTR)
			{
				continue;
			}
			
			DEBUG("connection lost in rfs_writev, size_sent: %u, %s\n", size_sent, strerror(errno));
			connection_lost = 1;
			return -errno;
		}
		
		size_sent += done;
	}
	
#ifdef RFS_DEBUG
	bytes_sent += size_sent;
	++send_operations;
#endif
	
	return size_sent;
}

static ssize_t rfs_readv(const int sock, struct iovec *iov, unsigned count)
{
	size_t overall_size = 0;
	int i = 0; for (i = 0; i < count; ++i)
	{
		overall_size += iov[i].iov_len;
	}
	
	DEBUG("receiving data: %u bytes\n", overall_size);
	
	ssize_t size_recv = 0;
	while (size_recv < overall_size)
	{
		count = fix_iov(iov, count, overall_size - size_recv);
		
		errno = 0;
		int done = readv(sock, iov, count);
		if (done < 0)
		{
			if (errno == EAGAIN || errno == EINTR)
			{
				continue;
			}
			
			DEBUG("connection lost in rfs_readv, size_recv: %u, %s\n", size_recv, strerror(errno));
			connection_lost = 1;
			return -errno;
		}
		
		size_recv += (size_t)done;
	}
	
#ifdef RFS_DEBUG
	bytes_received += size_recv;
	++receive_operations;
#endif
	
	return (size_t)size_recv;
}

size_t rfs_send_cmd(const int sock, const struct command *cmd)
{
	struct command send_command = { 0 };
	send_command.command = htonl(cmd->command);
	send_command.data_len = htonl(cmd->data_len);
	
	struct iovec iov[1] = { { 0, 0 } };
	iov[0].iov_base = (char*)&send_command;
	iov[0].iov_len  = sizeof(send_command);
	
	DEBUG("%s", "sending "); dump_command(cmd);
	ssize_t ret = rfs_writev(sock, iov, 1);
	if (ret < 0)
	{
		return -1;
	}
	DEBUG("%s\n", "done");
	
	return (size_t)ret;
}

size_t rfs_send_cmd_data(const int sock, const struct command *cmd, const void *data, const size_t data_len)
{
	struct command send_command = { 0 };
	send_command.command = htonl(cmd->command);
	send_command.data_len = htonl(cmd->data_len);
	
	struct iovec iov[2] = { { 0, 0 } };
	iov[0].iov_base = (char*)&send_command;
	iov[0].iov_len  = sizeof(send_command);
	iov[1].iov_base = (void*)data;
	iov[1].iov_len  = data_len;
	
	DEBUG("%s", "sending "); dump_command(cmd);
	ssize_t ret = rfs_writev(sock, iov, 2);
	if (ret < 0)
	{
		return -1;
	}
	DEBUG("%s\n", "done");
	
	return (size_t)ret;
}

size_t rfs_send_answer(const int sock, const struct answer *ans)
{
	struct answer send_answer = { 0 };
	send_answer.command = htonl(ans->command);
	send_answer.data_len = htonl(ans->data_len);
	send_answer.ret = htonl(ans->ret);
	send_answer.ret_errno = htons(ans->ret_errno);
	
	struct iovec iov[1] = { { 0, 0 } };
	iov[0].iov_base = (char*)&send_answer;
	iov[0].iov_len  = sizeof(send_answer);
	
	DEBUG("%s", "sending "); dump_answer(ans);
	ssize_t ret = rfs_writev(sock, iov, 1);
	if (ret < 0)
	{
		return -1;
	}
	DEBUG("%s\n", "done");
	
	return (size_t)ret;
}

size_t rfs_send_answer_data(const int sock, const struct answer *ans, const void *data, const size_t data_len)
{
	struct answer send_answer = { 0 };
	send_answer.command = htonl(ans->command);
	send_answer.data_len = htonl(ans->data_len);
	send_answer.ret = htonl(ans->ret);
	send_answer.ret_errno = htons(ans->ret_errno);
	
	struct iovec iov[2] = { { 0, 0 } };
	iov[0].iov_base = (char*)&send_answer;
	iov[0].iov_len  = sizeof(send_answer);
	iov[1].iov_base = (void*)data;
	iov[1].iov_len  = data_len;
	
	DEBUG("%s", "sending "); dump_answer(ans);
	ssize_t ret = rfs_writev(sock, iov, 2);
	if (ret < 0)
	{
		return -1;
	}
	DEBUG("%s\n", "done");
	
	return (size_t)ret;
}

size_t rfs_send_data(const int sock, const void *data, const size_t data_len)
{
	if (data_len < 1)
	{
		return 0;
	}
	
	struct iovec iov[1] = { { 0, 0 } };
	iov[0].iov_base = (char*)data;
	iov[0].iov_len  = data_len;

	ssize_t ret = rfs_writev(sock, iov, 1);
	if (ret < 0)
	{
		return -1;
	}
	
	return (size_t)ret;
}

size_t rfs_receive_answer(const int sock, struct answer *ans)
{
	struct answer recv_answer = { 0 };
	
	struct iovec iov[1] = { { 0, 0 } };
	iov[0].iov_base = (char*)&recv_answer;
	iov[0].iov_len  = sizeof(recv_answer);

	DEBUG("%s\n", "receiving answer");
	ssize_t ret = rfs_readv(sock, iov, 1);
	if (ret < 0)
	{
		return -1;
	}

	ans->command = ntohl(recv_answer.command);
	ans->data_len = ntohl(recv_answer.data_len);
	ans->ret = ntohl(recv_answer.ret);
	ans->ret_errno = ntohs(recv_answer.ret_errno);
	DEBUG("%s", "received "); dump_answer(ans);
	
	return (size_t)ret;
}

size_t rfs_receive_cmd(const int sock, struct command *cmd)
{
	struct command recv_command = { 0 };
	
	struct iovec iov[1] = { { 0, 0 } };
	iov[0].iov_base = (char*)&recv_command;
	iov[0].iov_len  = sizeof(recv_command);
	
	DEBUG("%s\n", "receiving command");
	ssize_t ret = rfs_readv(sock, iov, 1);
	if (ret < 0)
	{
		return -1;
	}
	
	cmd->command = ntohl(recv_command.command);
	cmd->data_len = ntohl(recv_command.data_len);
	
	DEBUG("%s", "received "); dump_command(cmd);
	
	return (size_t)ret;
}

size_t rfs_receive_data(const int sock, void *data, const size_t data_len)
{
	if (data_len < 1)
	{
		return 0;
	}
	
	struct iovec iov[1] = { { 0, 0 } };
	iov[0].iov_base = (char*)data;
	iov[0].iov_len  = data_len;

	ssize_t ret = rfs_readv(sock, iov, 1);
	if (ret < 0)
	{
		return -1;
	}
	
	return (size_t)ret;
}

size_t rfs_ignore_incoming_data(const int sock, const size_t data_len)
{
	size_t size_ignored = 0;
	char buffer[4096] = { 0 };
	
	struct iovec iov[1] = { { 0, 0 } };
	iov[0].iov_base = buffer;
	
	while (size_ignored < data_len)
	{
		iov[0].iov_len = (data_len - size_ignored > sizeof(buffer) ? sizeof(buffer) : data_len - size_ignored);
		
		ssize_t ret = rfs_readv(sock, iov, 1);
		if (ret < 1)
		{
			return -1;
		}
		
		size_ignored += (size_t)ret;
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

