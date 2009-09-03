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
#include <sys/uio.h>

#include "buffer.h"
#include "command.h"
#include "error.h"
#include "instance.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

int rfs_connect(struct sendrecv_info *info, const char *ip, unsigned port, unsigned force_ipv4, unsigned force_ipv6);

typedef struct iovec send_tok_entry;
typedef struct
{
	unsigned count;
	send_tok_entry iov[];
} send_tok;

#define MAKE_SEND_TOK(number) struct { unsigned count; struct iovec iov[number]; }

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

static inline size_t send_tok_size(unsigned count)
{
	return sizeof(send_tok) + count * sizeof(send_tok_entry);
}

static inline send_tok* send_token(unsigned count)
{
	send_tok *token = (send_tok *)get_buffer(send_tok_size(count));
	token->count = 0;
	memset(token->iov, 0, count * sizeof(send_tok_entry));
	return token;
}

static inline send_tok* queue_data(const char *buffer, size_t len, send_tok *token)
{
	DEBUG("data of size %lu\n", (unsigned long)len);
	if (token != NULL)
	{
		token->iov[token->count].iov_base = (void *)buffer;
		token->iov[token->count].iov_len = len;
		++(token->count);
	}
	return token;
}

static inline send_tok* queue_cmd(struct command *cmd, send_tok *token)
{
#ifdef RFS_DEBUG
	dump_command(cmd);
#endif
	if (token != NULL)
	{
		token->iov[token->count].iov_base = (void *)hton_cmd(cmd);
		token->iov[token->count].iov_len = sizeof(*cmd);
		++(token->count);
	}
	return token;
}

static inline send_tok* queue_ans(struct answer *ans, send_tok *token)
{
#ifdef RFS_DEBUG
	dump_answer(ans);
#endif
	if (token != NULL)
	{	
		token->iov[token->count].iov_base = (void *)hton_ans(ans);
		token->iov[token->count].iov_len = sizeof(*ans);
		++(token->count);
	}
	return token;
}

static inline send_tok* queue_16(uint16_t *value, send_tok *token)
{
	*value = htons(*value);
	return queue_data((char *)value, sizeof(*value), token);
}

static inline send_tok* queue_32(uint32_t *value, send_tok *token)
{
	*value = htonl(*value);
	return queue_data((char *)value, sizeof(*value), token);
}

static inline send_tok* queue_64(uint64_t *value, send_tok *token)
{
	*value = htonll(*value);
	return queue_data((char *)value, sizeof(*value), token);
}

static inline ssize_t do_send(struct sendrecv_info *info, send_tok *token)
{
	if (token == NULL)
	{
		return -EINVAL;
	}

	DEBUG("sending token with %u records\n", token->count);
	return rfs_writev(info, token->iov, token->count);
}

static inline ssize_t commit_send(struct sendrecv_info *info, send_tok *token)
{
	int send_ret = do_send(info, token);

	if (token != NULL)
	{
		free_buffer(token);
	}

	return send_ret;
}

static inline ssize_t rfs_send_data(struct sendrecv_info *info, const void *data, const size_t data_len)
{
	MAKE_SEND_TOK(1) token = { 1, {{ 0 }} };
	token.iov[0].iov_base = (void *)data;
	token.iov[0].iov_len = data_len;
	return (do_send(info, (send_tok *)(void *)&token) == data_len ? 0 : -1);
}

static inline ssize_t rfs_receive_data(struct sendrecv_info *info, void *data, const size_t data_len)
{
	return (rfs_recv(info, (char *)data, data_len, 0) == data_len ? 0 : -1);
}

ssize_t rfs_ignore_incoming_data(struct sendrecv_info *info, const size_t data_len);

#ifdef RFS_DEBUG
void dump_sendrecv_stats(struct sendrecv_info *info);
#endif

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* SENDRECV_H */

