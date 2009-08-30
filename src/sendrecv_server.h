/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef SENDRECV_SERVER_H
#define SENDRECV_SERVER_H

/** send/recv for server */

#include "sendrecv.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

static inline ssize_t rfs_send_answer(struct sendrecv_info *info, struct answer *ans)
{
#ifdef RFS_DEBUG
	dump_answer(ans);
#endif
	MAKE_SEND_TOK(1) token = { 1, {{ 0 }} };
	token.iov[0].iov_base = (void *)hton_ans(ans);
	token.iov[0].iov_len = sizeof(*ans);
	return (do_send(info, (send_tok *)(void *)&token) == sizeof(*ans) ? 0 : -1);
}

static inline ssize_t rfs_send_answer_oob(struct sendrecv_info *info, struct answer *ans)
{
	const char oob = 1;
	if (send(info->socket, &oob, 1, MSG_OOB) < 0)
	{
		return -1;
	}
	return rfs_send_answer(info, ans);
}

static inline ssize_t rfs_receive_cmd(struct sendrecv_info *info, struct command *cmd)
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

