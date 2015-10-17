/*
remotefs file system
See the file AUTHORS for copyright information.

This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#if defined SOLARIS
#include <sys/sockio.h>
#endif
#include <sys/socket.h>
#include <unistd.h>

#include "buffer.h"
#include "command.h"
#include "config.h"
#include "error.h"
#include "instance.h"
#include "sendrecv.h"

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

	unsigned i = 0; for (i = 0; i < count; ++i)
	{
		overall_size += iov[i].iov_len;
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
		DEBUG("count: %u, size left: %u, overall_size: %u, diff: %u\n", ret, (unsigned int)size_left, (unsigned int)overall_size, (unsigned int)diff);
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

ssize_t rfs_writev(rfs_sendrecv_info_t *info, struct iovec *iov, unsigned count)
{
	size_t overall_size = 0;
	unsigned i = 0; for (i = 0; i < count; ++i)
	{
		overall_size += iov[i].iov_len;
	}

	size_t size_sent = 0;
	while (size_sent < overall_size)
	{
		count = fix_iov(iov, count, overall_size - size_sent);

		if (count == 0)
		{
			return -EIO;
		}

		int done = 0;

		errno = 0;
		done = writev(info->socket, iov, count);

		if (done <= 0)
		{
#ifdef RFS_DEBUG
			++(info->send_interrupts);
#endif
			if (errno == EINTR)
			{
				continue;
			}

			if (errno != 0)
			{
				DEBUG("rfs_writev: %s (%d)\n", strerror(errno), errno);
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

static inline int is_mark(int socket, rfs_sendrecv_info_t *info)
{
	/* wait for further data become ready to not miss OOB byte */
	DEBUG("%s\n", "waiting for further data");

	fd_set read_set;

	FD_ZERO(&read_set);
	FD_SET(socket, &read_set);

	struct timeval timeout = { info->recv_timeout / 1000000,
		info->recv_timeout % 1000000 };

	DEBUG("%s\n", "selecting...");

	int select_ret = select(socket + 1, &read_set, NULL, NULL, info->recv_timeout > 0 ? &timeout : 0);

	if (select_ret < 0)
	{
		return -errno;
	}

	if (FD_ISSET(socket, &read_set) == 0)
	{
#ifdef RFS_DEBUG
		++(info->recv_timeouts);
#endif
		return -ETIMEDOUT;
	}

	int atmark = 0;

	errno = 0;
	if (ioctl(socket, SIOCATMARK, &atmark) < 0)
	{
		return -errno;
	}

	DEBUG("atmark: %d\n", atmark);

	if (atmark != 0)
	{
		char oob = 0;

		errno = 0;
		return (recv(socket, (char *)&oob, 1, MSG_OOB) < 0 ? -errno : 1);
	}

	return 0;
}

ssize_t rfs_recv(rfs_sendrecv_info_t *info, char *buffer, size_t size, unsigned check_oob)
{
	size_t size_recv = 0;
	while (size_recv < size)
	{
		errno = 0;
		ssize_t done = 0;
		if (check_oob != 0)
		{
			/* is_mark() will clear stream from OOB message */
			int check_mark = is_mark(info->socket, info);

			DEBUG("is mark: %d\n", check_mark);

			if (check_mark < 0)
			{
				info->connection_lost = 1;
				return check_mark;
			}
			else if (check_mark != 0)
			{
				info->oob_received = 1;
				return -EIO;
			}
		}

		errno = 0;
		done = recv(info->socket, buffer + size_recv, size - size_recv, MSG_WAITALL);

		if (done <= 0)
		{
#ifdef RFS_DEBUG
			++(info->recv_interrupts);
#endif

			if (errno == EINTR)
			{
				continue;
			}

			if (errno != 0) /* THE CAKE IS A LIE */
			{
				DEBUG("rfs_recv: %s (%d)\n", strerror(errno), errno);
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

	return size_recv;
}

ssize_t rfs_ignore_incoming_data(rfs_sendrecv_info_t *info, const size_t data_len)
{
	size_t size_ignored = 0;
	char buffer[4096] = { 0 };

	while (size_ignored < data_len)
	{
		ssize_t ret = rfs_recv(info,
			buffer,
			(data_len - size_ignored > sizeof(buffer) ? sizeof(buffer) : data_len - size_ignored),
			0);

		if (ret < 0)
		{
			return ret;
		}

		size_ignored += (size_t)ret;
	}

	return size_ignored;
}

#ifdef RFS_DEBUG
void dump_sendrecv_stats(rfs_sendrecv_info_t *info)
{
	DEBUG("%s\n", "dumping transfer statistics");
	DEBUG("### bytes sent: %lu (%.2fM, %.2fK)\n", info->bytes_sent, (float)info->bytes_sent / (1024 * 1024), (float)info->bytes_sent / 1024);
	DEBUG("### bytes recv: %lu (%.2fM, %.2fK)\n", info->bytes_recv, (float)info->bytes_recv / (1024 * 1024), (float)info->bytes_recv / 1024);
	DEBUG("### recv interrupts: %lu\n", info->recv_interrupts);
	DEBUG("### send interrupts: %lu\n", info->recv_interrupts);
	DEBUG("### recv timeouts: %lu\n", info->recv_timeouts);
	DEBUG("### send timeouts: %lu\n", info->send_timeouts);
	DEBUG("### conn timeouts: %lu\n", info->conn_timeouts);
}
#endif
