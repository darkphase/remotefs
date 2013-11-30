/*
remotefs file system
See the file AUTHORS for copyright information.

This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef SENDRECV_SERVER_H
#define SENDRECV_SERVER_H

/** send/recv for server */

#if defined SOLARIS || defined FREEBSD || defined QNX
#include <sys/socket.h>
#endif

#include "sendrecv.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

static inline struct command* ntoh_cmd(struct command *cmd)
{
	cmd->command = ntohl(cmd->command);
	cmd->data_len = ntohl(cmd->data_len);
	return cmd;
}

static inline send_token_t* queue_ans(struct answer *ans, send_token_t *token)
{
#ifdef RFS_DEBUG
	dump_answer(ans);
#endif
	return queue_buffer(answer, (char *)ans, sizeof(*ans), token);
}

static inline ssize_t rfs_send_answer(rfs_sendrecv_info_t *info, struct answer *ans)
{
#ifdef RFS_DEBUG
	dump_answer(ans);
#endif
	send_token_t token = { 0 };
	return (do_send(info,
		queue_ans(ans, &token
		)) == sizeof(*ans) ? 0 : -1);
}

static inline ssize_t rfs_send_answer_oob(rfs_sendrecv_info_t *info, struct answer *ans)
{
	const char oob = 1;
	if (send(info->socket, &oob, 1, MSG_OOB) < 0)
	{
		return -1;
	}
	return rfs_send_answer(info, ans);
}

static inline ssize_t rfs_receive_cmd(rfs_sendrecv_info_t *info, struct command *cmd)
{
	ssize_t ret = rfs_recv(info, (char *)cmd, sizeof(*cmd), 0);
	if (ret < 0)
	{
		return -1;
	}

	ntoh_cmd(cmd);
#ifdef RFS_DEBUG
	dump_command(cmd);
#endif
	return (ret == sizeof(*cmd) ? 0 : -1);
}

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* SENDRECV_SERVER_H */
