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

	struct timeval to = { 3, 0 };
	int ret;
	ret=setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &to,  sizeof(to));
#if RFS_DEBUG
	if ( ret )
		perror("setsockopt SO_RCVTIMEO");
#endif
	ret=setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &to,  sizeof(to));
#if RFS_DEBUG
	if ( ret )
		perror("setsockopt SO_SNSTIMEO");
#endif
	
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

	struct timeval to = { 3, 0 };
	int ret;
	ret=setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &to,  sizeof(to));
#if RFS_DEBUG
	if ( ret )
		perror("setsockopt SO_RCVTIMEO");
#endif
	ret=setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &to,  sizeof(to));
#if RFS_DEBUG
	if ( ret )
		perror("setsockopt SO_SNSTIMEO");
#endif

	if (connect(sock, (struct sockaddr *)res->ai_addr, res->ai_addrlen) == -1)
	{
		freeaddrinfo(res);
		return -errno;
	}
#endif
	g_server_socket = sock;
	
	return sock;
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
	
	DEBUG("%s\n", "done");
	
	return ret;
}

size_t rfs_send_cmd_data(const int sock, const struct command *cmd, const void *data, const size_t data_len)
{
	size_t size_sent = 0;
	struct iovec iov[2];
	struct command send_command = { 0 };
	send_command.command = htonl(cmd->command);
	send_command.data_len = htonl(cmd->data_len);
	
	iov[0].iov_base = (char*)&send_command;
	iov[0].iov_len  = sizeof(send_command);
	iov[1].iov_base = (void*)data;
	iov[1].iov_len  = data_len;
	
	DEBUG("%s", "sending "); dump_command(cmd);
	DEBUG("sending data: %u bytes\n", data_len);

	errno=0;
	size_sent = writev(sock, iov, 2);

	if (size_sent < 0)
	{
		connection_lost = 1;
	}
	
	DEBUG("%s\n", "done");
	
#ifdef RFS_DEBUG
	bytes_sent += size_sent;
	++send_operations;
#endif
	
	return size_sent;
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
	
	DEBUG("%s\n", "done");
	
	return ret;
}

size_t rfs_send_answer_data(const int sock, const struct answer *ans, const void *data, const size_t data_len)
{
	size_t size_sent = 0;
	struct iovec iov[2];
	struct answer send_answer = { 0 };
	send_answer.command = htonl(ans->command);
	send_answer.data_len = htonl(ans->data_len);
	send_answer.ret = htonl(ans->ret);
	send_answer.ret_errno = htons(ans->ret_errno);
	
	DEBUG("%s", "sending "); dump_answer(ans);
	DEBUG("sending data: %u bytes\n", data_len);
	iov[0].iov_base = (char*)&send_answer;
	iov[0].iov_len  = sizeof(send_answer);
	iov[1].iov_base = (void*)data;
	iov[1].iov_len  = data_len;

	errno=0;
	size_sent = writev(sock, iov, 2);
	if (size_sent < 0)
	{
		connection_lost = 1;
	}
	
	DEBUG("%s\n", "done");
	
#ifdef RFS_DEBUG
	bytes_sent += size_sent;
	++send_operations;
#endif
	
	return size_sent;
}

size_t rfs_send_data(const int sock, const void *data, const size_t data_len)
{
	DEBUG("sending data: %u bytes\n", data_len);
	size_t size_sent = 0;
	
	while (size_sent < data_len)
	{
		errno=0;
		int done = send(sock, (const char *)data + size_sent, data_len - size_sent, 0);
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
	
	DEBUG("%s\n", "done");
	
	return size_sent;
}

size_t rfs_receive_answer(const int sock, struct answer *ans)
{
	DEBUG("%s\n", "receiving answer");
	
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
	DEBUG("%s\n", "receiving command");

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
	DEBUG("receiving %d bytes\n", data_len);

	size_t size_received = 0;
	while (size_received < data_len)
	{
		errno=0;
		int done = recv(sock, (char *)data + size_received, data_len - size_received, 0);
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

	DEBUG("%s\n", "done");

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

