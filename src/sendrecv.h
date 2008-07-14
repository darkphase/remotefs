#ifndef SEND_H
#define SEND_H

/* socket send/recv and connect/disconnect routines */

#include "config.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct command;
struct answer;

int rfs_connect(const char *ip, const unsigned port);
void rfs_disconnect(int sock, int gently);

size_t rfs_send_cmd(const int sock, const struct command *cmd);
size_t rfs_send_answer(const int sock, const struct answer *ans);
size_t rfs_send_data(const int sock, const void *data, const size_t data_len);
size_t rfs_receive_cmd(const int sock, struct command *cmd);
size_t rfs_receive_answer(const int sock, struct answer *ans);
size_t rfs_receive_data(const int sock, void *data, const size_t data_len);

size_t rfs_ignore_incoming_data(const int sock, const size_t data_len);
int rfs_is_connection_lost();
void rfs_set_connection_restored();

#ifdef RFS_DEBUG
void dump_sendrecv_stats();
#endif

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif // SEND_H
