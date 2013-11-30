/*
remotefs file system
See the file AUTHORS for copyright information.

This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef SENDRECV_CLIENT_H
#define SENDRECV_CLIENT_H

/** send/recv for client */

#include "sendrecv.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

static inline struct answer* ntoh_ans(struct answer *ans)
{
	ans->command = ntohl(ans->command);
	ans->data_len = ntohl(ans->data_len);
	ans->ret = ntohl(ans->ret);
	ans->ret_errno = ntoh_errno(ans->ret_errno);
	return ans;
}

static inline send_token_t* queue_cmd(struct command *cmd, send_token_t *token)
{
#ifdef RFS_DEBUG
	dump_command(cmd);
#endif
	return queue_buffer(command, (char *)cmd, sizeof(*cmd), token);
}

static inline ssize_t rfs_send_cmd(rfs_sendrecv_info_t *info, struct command *cmd)
{
#ifdef RFS_DEBUG
	dump_command(cmd);
#endif
	send_token_t token = { 0 };
	return (do_send(info,
		queue_cmd(cmd, &token
		)) == sizeof(*cmd) ? 0 : -1);
}

static inline ssize_t rfs_receive_data_oob(rfs_sendrecv_info_t *info, void *data, const size_t data_len)
{
	ssize_t ret = rfs_recv(info, (char *)data, data_len, 1);
	return ((ret < 0 || (size_t)ret != data_len) ? -1 : 0);
}

static inline ssize_t rfs_receive_answer(rfs_sendrecv_info_t *info, struct answer *ans)
{
	ssize_t ret = rfs_recv(info, (char *)ans, sizeof(*ans), 0);
	if (ret < 0)
	{
		return -1;
	}

	ntoh_ans(ans);
#ifdef RFS_DEBUG
	dump_answer(ans);
#endif
	return (ret == sizeof(*ans) ? 0 : -1);
}

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* SENDRECV_CLIENT_H */
