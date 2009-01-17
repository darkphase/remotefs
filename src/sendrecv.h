/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef SEND_H
#define SEND_H

/** socket send/recv and connect/disconnect routines */

#include "config.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct command;
struct answer;
struct sendrecv_info;

int rfs_connect(struct sendrecv_info *info, const char *ip, const unsigned port);

size_t rfs_send_cmd(struct sendrecv_info *info, const struct command *cmd);
size_t rfs_send_answer(struct sendrecv_info *info, const struct answer *ans);
size_t rfs_send_answer_oob(struct sendrecv_info *info, const struct answer *ans);
size_t rfs_send_data(struct sendrecv_info *info, const void *data, const size_t data_len);
size_t rfs_receive_cmd(struct sendrecv_info *info, struct command *cmd);
size_t rfs_receive_answer(struct sendrecv_info *info, struct answer *ans);
size_t rfs_receive_data(struct sendrecv_info *info, void *data, const size_t data_len);
size_t rfs_send_cmd_data(struct sendrecv_info *info, const struct command *cmd, const void *data, const size_t data_len);
size_t rfs_send_cmd_data2(struct sendrecv_info *info, const struct command *cmd, const void *data, const size_t data_len, const void *data2, const size_t data_len2);
size_t rfs_send_answer_data(struct sendrecv_info *info, const struct answer *ans, const void *data, const size_t data_len);

size_t rfs_ignore_incoming_data(struct sendrecv_info *info, const size_t data_len);

#ifdef RFS_DEBUG
void dump_sendrecv_stats(struct sendrecv_info *info);
#endif

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* SEND_H */
