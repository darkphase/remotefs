/*
remotefs file system
See the file AUTHORS for copyright information.

This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef SENDRECV_H
#define SENDRECV_H

/** socket send/recv and connect/disconnect routines */

#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <sys/uio.h>
#include "buffer.h"
#include "command.h"
#include "compat.h"
#include "error.h"
#include "instance.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum data_widths { none = 0, command, answer, w16 = 16, w32 = 32, w64 = 64 };

#define W16 sizeof(uint16_t)
#define W32 sizeof(uint32_t)
#define W64 sizeof(uint64_t)

typedef struct iovec send_token_entry_t;

/** IOV_MAX iovectors max */
typedef struct
{
	unsigned count;
	enum data_widths data_width[IOV_MAX];
	send_token_entry_t iov[IOV_MAX];
} send_token_t;

/* low lev */
ssize_t rfs_writev(rfs_sendrecv_info_t *info, struct iovec *iov, unsigned count);
ssize_t rfs_recv(rfs_sendrecv_info_t *info, char *buffer, size_t size, unsigned check_oob);

/* common */

static inline send_token_t* queue_buffer(enum data_widths width, const char *buffer, size_t len, send_token_t *token)
{
	/*DEBUG("data of size %lu\n", (unsigned long)len);*/
	if (token != NULL && len > 0)
	{
		if (token->count >= IOV_MAX)
		{
			return NULL;
		}

		token->data_width[token->count] = width;
		token->iov[token->count].iov_base = (void *)buffer;
		token->iov[token->count].iov_len = len;
		++(token->count);
	}
	return token;
}

static inline send_token_t* queue_data(const char *buffer, size_t len, send_token_t *token)
{
	return queue_buffer(none, buffer, len, token);
}

static inline send_token_t* queue_16(uint16_t *value, send_token_t *token)
{
	return queue_buffer(w16, (const char *)value, sizeof(*value), token);
}

static inline send_token_t* queue_32(uint32_t *value, send_token_t *token)
{
	return queue_buffer(w32, (const char *)value, sizeof(*value), token);
}

static inline send_token_t* queue_64(uint64_t *value, send_token_t *token)
{
	return queue_buffer(w64, (const char *)value, sizeof(*value), token);
}

static inline ssize_t do_send(rfs_sendrecv_info_t *info, send_token_t *token)
{
	if (token == NULL)
	{
		return -EINVAL;
	}

	size_t i = 0; for (; i < token->count; ++i)
	{
		switch (token->data_width[i])
		{
		case command:
			{
			struct command *cmd = (struct command *)(token->iov[i].iov_base);
			cmd->command = htonl(cmd->command);
			cmd->data_len = htonl(cmd->data_len);
			}
			break;
		case answer:
			{
			struct answer *ans = (struct answer *)(token->iov[i].iov_base);
			ans->command = htonl(ans->command);
			ans->data_len = htonl(ans->data_len);
			ans->ret = htonl(ans->ret);
			ans->ret_errno = hton_errno(ans->ret_errno);
			}
			break;
		case w16:
			*((uint16_t *)token->iov[i].iov_base) = htons(*((uint16_t *)token->iov[i].iov_base));
			break;
		case w32:
			*((uint32_t *)token->iov[i].iov_base) = htonl(*((uint32_t *)token->iov[i].iov_base));
			break;
		case w64:
			*((uint64_t *)token->iov[i].iov_base) = htonll(*((uint64_t *)token->iov[i].iov_base));
			break;
		default:
			/*DEBUG("skipping iov %lu\n", (unsigned long)i);*/
			break;
		}
	}

	DEBUG("sending token with %u records\n", token->count);
	return rfs_writev(info, token->iov, token->count);
}

static inline ssize_t rfs_receive_data(rfs_sendrecv_info_t *info, void *data, const size_t data_len)
{
	return ((size_t)(rfs_recv(info, (char *)data, data_len, 0)) == data_len ? 0 : -1);
}

ssize_t rfs_ignore_incoming_data(rfs_sendrecv_info_t *info, const size_t data_len);

#ifdef RFS_DEBUG
void dump_sendrecv_stats(rfs_sendrecv_info_t *info);
#endif

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* SENDRECV_H */
