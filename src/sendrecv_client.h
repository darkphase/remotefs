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

static inline ssize_t rfs_send_cmd(struct sendrecv_info *info, struct command *cmd)
{
#ifdef RFS_DEBUG
	dump_command(cmd);
#endif
	send_token_t token = { 1, {{ 0 }} };
	token.iov[0].iov_base = (void *)hton_cmd(cmd);
	token.iov[0].iov_len = sizeof(*cmd);
	return (do_send(info, &token) == sizeof(*cmd) ? 0 : -1);
}

static inline ssize_t rfs_receive_data_oob(struct sendrecv_info *info, void *data, const size_t data_len)
{
	return (rfs_recv(info, (char *)data, data_len, 1) == data_len ? 0 : -1);
}

static inline ssize_t rfs_receive_answer(struct sendrecv_info *info, struct answer *ans)
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
