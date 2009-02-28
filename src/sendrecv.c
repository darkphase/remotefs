/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <netdb.h>
#include <string.h>
#if defined SOLARIS
#include <sys/sockio.h>
#endif
#ifdef RFS_DEBUG
#include <sys/time.h>
#endif

#include "command.h"
#include "config.h"
#include "error.h"
#include "instance.h"
#ifdef WITH_SSL
#include "ssl.h"
#endif

int rfs_connect(struct sendrecv_info *info, const char *ip, const unsigned port)
{
	struct addrinfo *addr_info = NULL;
	struct addrinfo hints = { 0 };

	/* resolve name or address */
	hints.ai_family    = AF_UNSPEC;
	hints.ai_socktype  = SOCK_STREAM; 
	hints.ai_flags     = AI_ADDRCONFIG;
	
	int result = getaddrinfo(ip, NULL, &hints, &addr_info);
	if (result != 0)
	{
		/*ERROR("Can't resolve address for %s : %s\n", ip, gai_strerror(result));*/
		return -EHOSTUNREACH;
	}
	
	errno = 0;
	int sock = -1;

	if (addr_info->ai_family == AF_INET)
	{
		struct sockaddr_in *addr = (struct sockaddr_in *)addr_info->ai_addr;
		addr->sin_port = htons(port);
		sock = socket(AF_INET, SOCK_STREAM, 0);
	}
#ifdef WITH_IPV6
	else
	{
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr_info->ai_addr;
		addr6->sin6_port = htons(port);
		sock = socket(AF_INET6, SOCK_STREAM, 0);
	}
#endif
	
	if (sock == -1)
	{
		freeaddrinfo(addr_info);
		return -errno;
	}

	errno = 0;
	if (connect(sock, (struct sockaddr *)addr_info->ai_addr, addr_info->ai_addrlen) == -1)
	{
		freeaddrinfo(addr_info);
		return -errno;
	}
	
	freeaddrinfo(addr_info);
	
	info->socket = sock;
	info->connection_lost = 0;
	
	return sock;
}

#ifdef RFS_DEBUG
static void dump_iov(struct iovec *iov, unsigned count)
{
	DEBUG("dumping %u io vectors:\n", count);
	unsigned i = 0; for (i = 0; i < count; ++i)
	{
		DEBUG("%u: base: %p, len: %u\n", i, iov[i].iov_base, (unsigned int)iov[i].iov_len);
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
	
	int ret = count;
	while (1)
	{
#ifdef RFS_DEBUG
		DEBUG("count: %u, size left: %u, overall_size: %u, diff: %u\n", count, (unsigned int)size_left, (unsigned int)overall_size, (unsigned int)diff);
		dump_iov(iov, ret);
#endif
		if (ret < 1)
		{
			DEBUG("%s\n", "error while fixing iovs");
			break; /* actually error */
		}
		
		if (iov[0].iov_len > diff)
		{
			iov[0].iov_base = (char*)iov[0].iov_base + diff;
			iov[0].iov_len -= diff;
			break;
		}
		else if (diff != 0)
		{
			diff -= iov[0].iov_len;
			
			int j = 0; for (j = 0; j < ret; ++j)
			{
				iov[j].iov_len = iov[j + 1].iov_len;
				iov[j].iov_base = iov[j + 1].iov_base;
			}
			
			--ret;
			
			continue;
		}
	}
	
#ifdef RFS_DEBUG
	dump_iov(iov, ret);
#endif
	
	return ret;
}

static ssize_t rfs_writev(struct sendrecv_info *info, struct iovec *iov, unsigned count)
{
	size_t overall_size = 0;
	int i = 0; for (i = 0; i < count; ++i)
	{
		overall_size += iov[i].iov_len;
	}
	
	ssize_t size_sent = 0;
	while (size_sent < overall_size)
	{
		count = fix_iov(iov, count, overall_size - size_sent);
		
		if (count == 0)
		{
			return -EIO;
		}
		
		int done = 0;
		
		errno = 0;
#ifdef RFS_DEBUG
		struct timeval start_time = { 0 };
		struct timeval stop_time = { 0 };
		
		gettimeofday(&start_time, NULL);
#endif
#ifdef WITH_SSL
		if (info->ssl_enabled != 0)
		{
			done = rfs_ssl_write(info->ssl_socket, iov[0].iov_base, iov[0].iov_len);
		}
		else
		{
#endif
			done = writev(info->socket, iov, count);
#ifdef WITH_SSL
		}
#endif
#ifdef RFS_DEBUG
		
		gettimeofday(&stop_time, NULL);
		
		info->send_susecs_used += ((stop_time.tv_sec * 1000000 + stop_time.tv_usec) 
		- (start_time.tv_sec * 1000000 + start_time.tv_usec));
#endif
		if (done <= 0)
		{
			if (errno == EAGAIN || errno == EINTR)
			{
				continue;
			}
			
			if (errno != 0)
			{
				DEBUG("%s encountered\n", strerror(errno));
			}
			
			DEBUG("connection lost in rfs_writev, size_sent: %u, %s\n", (unsigned int)size_sent, strerror(errno));
			info->connection_lost = 1;
			/* see comment for rfs_recv */
			return errno == 0 ? -ECONNABORTED : -errno;
		}
		
		size_sent += done;
	}
	
#ifdef RFS_DEBUG
	if (size_sent > 0)
	{
		info->bytes_sent += size_sent;
	}
#endif
	
	return size_sent;
}

static ssize_t rfs_recv(struct sendrecv_info *info, char *buffer, size_t size)
{
	ssize_t size_recv = 0;
	while (size_recv < size)
	{
		errno = 0;
		ssize_t done = 0;
#ifdef RFS_DEBUG
		struct timeval start_time = { 0 };
		struct timeval stop_time = { 0 };
		
		gettimeofday(&start_time, NULL);
#endif
#ifdef WITH_SSL
		if (info->ssl_enabled != 0)
		{
			DEBUG("%s\n", "using SSL for read");
			done = rfs_ssl_read(info->ssl_socket, buffer + size_recv, size - size_recv);
		}
		else
		{
#endif
			done = recv(info->socket, buffer + size_recv, size - size_recv, 0);
#ifdef WITH_SSL
		}
#endif
#ifdef RFS_DEBUG
		gettimeofday(&stop_time, NULL);
		
		info->recv_susecs_used += ((stop_time.tv_sec * 1000000 + stop_time.tv_usec) 
		- (start_time.tv_sec * 1000000 + start_time.tv_usec));
#endif
		if (done <= 0)
		{
			if (errno == EAGAIN || errno == EINTR)
			{
				continue;
			}
			
			if (errno != 0) /* THE CAKE IS A LIE */
			{
				DEBUG("%s encountered\n", strerror(errno));
			}
			
			DEBUG("connection lost in rfs_recv, size_recv: %d, %s\n", (int)size_recv, strerror(errno));
			info->connection_lost = 1;
			/* if connection lost, then we could get Success
			so fake ret code for upper layer to be able to recognize error */
			return errno == 0 ? -ECONNABORTED : -errno;
		}
		
		size_recv += (size_t)done;
	}
	
#ifdef RFS_DEBUG
	info->bytes_recv += size_recv;
#endif
	
	return (size_t)size_recv;
}

static ssize_t rfs_readv(struct sendrecv_info *info, struct iovec *iov, unsigned count)
{
	/* readv is somewhat strange 
	it's receiving data by chunks of 1460 bytes (wtf?) and sometimes hangs
	not sure if hanging isn't remotefs fault 
	
	but it's better to use rfs_recv for now, since we're not using
	scatter input anyway. yet */
	
	if (count == 1)
	{
		return rfs_recv(info, iov[0].iov_base, iov[0].iov_len);
	}

	return -EINVAL;
}

size_t rfs_send_cmd(struct sendrecv_info *info, const struct command *cmd)
{
	struct command send_command = { 0 };
	send_command.command = htonl(cmd->command);
	send_command.data_len = htonl(cmd->data_len);
	
	struct iovec iov[1] = { { 0, 0 } };
	iov[0].iov_base = (char*)&send_command;
	iov[0].iov_len  = sizeof(send_command);
	
#ifdef RFS_DEBUG
	DEBUG("%s", "sending "); dump_command(cmd);
#endif

	ssize_t ret = rfs_writev(info, iov, 1);
	if (ret < 0)
	{
		return -1;
	}
	DEBUG("%s\n", "done");
	
	return (size_t)ret;
}

size_t rfs_send_cmd_data(struct sendrecv_info *info, const struct command *cmd, const void *data, const size_t data_len)
{
	struct command send_command = { 0 };
	send_command.command = htonl(cmd->command);
	send_command.data_len = htonl(cmd->data_len);
	
	struct iovec iov[2] = { { 0, 0 } };
	iov[0].iov_base = (char*)&send_command;
	iov[0].iov_len  = sizeof(send_command);
	iov[1].iov_base = (void*)data;
	iov[1].iov_len  = data_len;
	
#ifdef RFS_DEBUG
	DEBUG("%s", "sending "); dump_command(cmd);
#endif

	ssize_t ret = rfs_writev(info, iov, 2);
	if (ret < 0)
	{
		return -1;
	}
	DEBUG("%s\n", "done");
	
	return (size_t)ret;
}

size_t rfs_send_cmd_data2(struct sendrecv_info *info, const struct command *cmd, const void *data, const size_t data_len, const void *data2, const size_t data_len2)
{
	struct command send_command = { 0 };
	send_command.command = htonl(cmd->command);
	send_command.data_len = htonl(cmd->data_len);
	
	struct iovec iov[3] = { { 0, 0 } };
	iov[0].iov_base = (char*)&send_command;
	iov[0].iov_len  = sizeof(send_command);
	iov[1].iov_base = (void*)data;
	iov[1].iov_len  = data_len;
	iov[2].iov_base = (void*)data2;
	iov[2].iov_len  = data_len2;
	
#ifdef RFS_DEBUG
	DEBUG("%s", "sending "); dump_command(cmd);
#endif

	ssize_t ret = rfs_writev(info, iov, 3);
	if (ret < 0)
	{
		return -1;
	}
	DEBUG("%s\n", "done");
	
	return (size_t)ret;
}

size_t rfs_send_answer(struct sendrecv_info *info, const struct answer *ans)
{
	struct answer send_answer = { 0 };
	send_answer.command = htonl(ans->command);
	send_answer.data_len = htonl(ans->data_len);
	send_answer.ret = htonl(ans->ret);
	send_answer.ret_errno = hton_errno(ans->ret_errno);
	
	struct iovec iov[1] = { { 0, 0 } };
	iov[0].iov_base = (char*)&send_answer;
	iov[0].iov_len  = sizeof(send_answer);
	
#ifdef RFS_DEBUG
	DEBUG("%s", "sending "); dump_answer(ans);
#endif

	ssize_t ret = rfs_writev(info, iov, 1);
	if (ret < 0)
	{
		return -1;
	}
	DEBUG("%s\n", "done");
	
	return (size_t)ret;
}

size_t rfs_send_answer_data(struct sendrecv_info *info, const struct answer *ans, const void *data, const size_t data_len)
{
	struct answer send_answer = { 0 };
	send_answer.command = htonl(ans->command);
	send_answer.data_len = htonl(ans->data_len);
	send_answer.ret = htonl(ans->ret);
	send_answer.ret_errno = hton_errno(ans->ret_errno);
	
	struct iovec iov[2] = { { 0, 0 } };
	iov[0].iov_base = (char*)&send_answer;
	iov[0].iov_len  = sizeof(send_answer);
	iov[1].iov_base = (void*)data;
	iov[1].iov_len  = data_len;
	
#ifdef RFS_DEBUG
	DEBUG("%s", "sending "); dump_answer(ans);
#endif

	ssize_t ret = rfs_writev(info, iov, 2);
	if (ret < 0)
	{
		return -1;
	}
	DEBUG("%s\n", "done");
	
	return (size_t)ret;
}

size_t rfs_send_answer_oob(struct sendrecv_info *info, const struct answer *ans)
{
	DEBUG("%s\n", "sending oob");
	const char oob = 1;
	if (send(info->socket, &oob, 1, MSG_OOB) < 0)
	{
		return -1;
	}
	
	size_t size_sent = 0;

	struct answer send_answer = { 0 };
	send_answer.command = htonl(ans->command);
	send_answer.data_len = htonl(ans->data_len);
	send_answer.ret = htonl(ans->ret);
	send_answer.ret_errno = hton_errno(ans->ret_errno);
	
	struct iovec iov[1] = { { 0, 0 } };
	iov[0].iov_base = (char*)&send_answer;
	iov[0].iov_len  = sizeof(send_answer);
	
#ifdef RFS_DEBUG
	DEBUG("%s", "sending "); dump_answer(ans);
#endif

	ssize_t ret = rfs_writev(info, iov, 1);
	if (ret < 0)
	{
		return -1;
	}
	DEBUG("%s\n", "done");

	return size_sent;
}

size_t rfs_send_data(struct sendrecv_info *info, const void *data, const size_t data_len)
{
	if (data_len < 1)
	{
		return 0;
	}
	
	struct iovec iov[1] = { { 0, 0 } };
	iov[0].iov_base = (char*)data;
	iov[0].iov_len  = data_len;

	ssize_t ret = rfs_writev(info, iov, 1);
	if (ret < 0)
	{
		return -1;
	}
	
	return (size_t)ret;
}

size_t rfs_receive_answer(struct sendrecv_info *info, struct answer *ans)
{
	struct answer recv_answer = { 0 };
	
	struct iovec iov[1] = { { 0, 0 } };
	iov[0].iov_base = (char*)&recv_answer;
	iov[0].iov_len  = sizeof(recv_answer);

	DEBUG("%s\n", "receiving answer");
	ssize_t ret = rfs_readv(info, iov, 1);
	if (ret < 0)
	{
		return -1;
	}

	ans->command = ntohl(recv_answer.command);
	ans->data_len = ntohl(recv_answer.data_len);
	ans->ret = ntohl(recv_answer.ret);
	ans->ret_errno = ntoh_errno(recv_answer.ret_errno);

#ifdef RFS_DEBUG
	DEBUG("%s", "received "); dump_answer(ans);
#endif
	
	return (size_t)ret;
}

size_t rfs_receive_cmd(struct sendrecv_info *info, struct command *cmd)
{
	struct command recv_command = { 0 };
	
	struct iovec iov[1] = { { 0, 0 } };
	iov[0].iov_base = (char*)&recv_command;
	iov[0].iov_len  = sizeof(recv_command);
	
	DEBUG("%s\n", "receiving command");
	ssize_t ret = rfs_readv(info, iov, 1);
	if (ret < 0)
	{
		return -1;
	}
	
	cmd->command = ntohl(recv_command.command);
	cmd->data_len = ntohl(recv_command.data_len);
	
#ifdef RFS_DEBUG
	DEBUG("%s", "received "); dump_command(cmd);
#endif
	
	return (size_t)ret;
}

size_t rfs_receive_data(struct sendrecv_info *info, void *data, const size_t data_len)
{
	if (data_len < 1)
	{
		return 0;
	}
	
	struct iovec iov[1] = { { 0, 0 } };
	iov[0].iov_base = (char*)data;
	iov[0].iov_len  = data_len;

	ssize_t ret = rfs_readv(info, iov, 1);
	if (ret < 0)
	{
		return -1;
	}
	
	return (size_t)ret;
}

size_t rfs_ignore_incoming_data(struct sendrecv_info *info, const size_t data_len)
{
	size_t size_ignored = 0;
	char buffer[4096] = { 0 };
	
	struct iovec iov[1] = { { 0, 0 } };
	iov[0].iov_base = buffer;
	
	while (size_ignored < data_len)
	{
		iov[0].iov_len = (data_len - size_ignored > sizeof(buffer) ? sizeof(buffer) : data_len - size_ignored);
		
		ssize_t ret = rfs_readv(info, iov, 1);
		if (ret < 1)
		{
			return -1;
		}
		
		size_ignored += (size_t)ret;
	}
	
	return size_ignored;
}

#ifdef RFS_DEBUG
void dump_sendrecv_stats(struct sendrecv_info *info)
{
	DEBUG("%s\n", "dumping transfer statistics");
	DEBUG("bytes sent: %lu (%.2fM, %.2fK)\n", info->bytes_sent, (float)info->bytes_sent / (1024 * 1024), (float)info->bytes_sent / 1024);
	DEBUG("bytes recv: %lu (%.2fM, %.2fK)\n", info->bytes_recv, (float)info->bytes_recv / (1024 * 1024), (float)info->bytes_recv / 1024);
	DEBUG("secs used for sending: %.2f (%.2fMb/s, %.2fKb/s)\n", info->send_susecs_used / 1000000.0, 
	info->send_susecs_used != 0 ? (info->bytes_sent / (1024 * 1024)) / (info->send_susecs_used / 1000000.0) : 0,
	info->send_susecs_used != 0 ? (info->bytes_sent / 1024) / (info->send_susecs_used / 1000000.0) : 0);
	DEBUG("secs used for receiving: %.2f (%.2fMb/s, %.2fKb/s)\n", info->recv_susecs_used / 1000000.0, 
	info->recv_susecs_used != 0 ? (info->bytes_recv / (1024 * 1024)) / (info->recv_susecs_used / 1000000.0) : 0,
	info->recv_susecs_used != 0 ? (info->bytes_recv / 1024) / (info->recv_susecs_used / 1000000.0) : 0);
}
#endif

