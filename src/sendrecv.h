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
#if defined QNX && ! defined IOV_MAX
#define IOV_MAX 16
#endif
#include "buffer.h"
#include "command.h"
#include "compat.h"
#include "error.h"
#include "instance.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct iovec send_token_entry_t;

/** IOV_MAX iovectors max */
typedef struct
{
	unsigned count;
	send_token_entry_t iov[IOV_MAX];
} send_token_t;

/* low lev */

ssize_t rfs_writev(struct sendrecv_info *info, struct iovec *iov, unsigned count);
ssize_t rfs_recv(struct sendrecv_info *info, char *buffer, size_t size, unsigned check_oob);

/* utils */

static inline struct command* hton_cmd(struct command *cmd)
{
	cmd->command = htonl(cmd->command);
	cmd->data_len = htonl(cmd->data_len);
	return cmd;
}

static inline struct command* ntoh_cmd(struct command *cmd)
{
	cmd->command = ntohl(cmd->command);
	cmd->data_len = ntohl(cmd->data_len);
	return cmd;
}

static inline struct answer* hton_ans(struct answer *ans)
{
	ans->command = htonl(ans->command);
	ans->data_len = htonl(ans->data_len);
	ans->ret = htonl(ans->ret);
	ans->ret_errno = hton_errno(ans->ret_errno);
	return ans;
}

static inline struct answer* ntoh_ans(struct answer *ans)
{
	ans->command = ntohl(ans->command);
	ans->data_len = ntohl(ans->data_len);
	ans->ret = ntohl(ans->ret);
	ans->ret_errno = ntoh_errno(ans->ret_errno);
	return ans;
}

/* common */

static inline send_token_t* queue_data(const char *buffer, size_t len, send_token_t *token)
{
	DEBUG("data of size %lu\n", (unsigned long)len);
	if (token != NULL && len > 0)
	{
		if (token->count >= IOV_MAX)
		{
			return NULL;
		}

		token->iov[token->count].iov_base = (void *)buffer;
		token->iov[token->count].iov_len = len;
		++(token->count);
	}
	return token;
}

static inline send_token_t* queue_cmd(struct command *cmd, send_token_t *token)
{
#ifdef RFS_DEBUG
	dump_command(cmd);
#endif
	return queue_data((char *)hton_cmd(cmd), sizeof(*cmd), token);
}

static inline send_token_t* queue_ans(struct answer *ans, send_token_t *token)
{
#ifdef RFS_DEBUG
	dump_answer(ans);
#endif
	return queue_data((char *)hton_ans(ans), sizeof(*ans), token);
}

static inline send_token_t* queue_16(uint16_t *value, send_token_t *token)
{
	*value = htons(*value);
	return queue_data((char *)value, sizeof(*value), token);
}

static inline send_token_t* queue_32(uint32_t *value, send_token_t *token)
{
	*value = htonl(*value);
	return queue_data((char *)value, sizeof(*value), token);
}

static inline send_token_t* queue_64(uint64_t *value, send_token_t *token)
{
	*value = htonll(*value);
	return queue_data((char *)value, sizeof(*value), token);
}

static inline ssize_t do_send(struct sendrecv_info *info, send_token_t *token)
{
	if (token == NULL)
	{
		return -EINVAL;
	}

	DEBUG("sending token with %u records\n", token->count);
	return rfs_writev(info, token->iov, token->count);
}

static inline ssize_t rfs_receive_data(struct sendrecv_info *info, void *data, const size_t data_len)
{
	return ((size_t)(rfs_recv(info, (char *)data, data_len, 0)) == data_len ? 0 : -1);
}

ssize_t rfs_ignore_incoming_data(struct sendrecv_info *info, const size_t data_len);

#ifdef RFS_DEBUG
void dump_sendrecv_stats(struct sendrecv_info *info);
#endif

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* SENDRECV_H */
