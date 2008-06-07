#ifndef SEND_H
#define SEND_H

#include "config.h"

struct command;
struct answer;

int rfs_connect(const char *ip, const unsigned port);
void rfs_disconnect(int sock);

size_t rfs_send_cmd(const int sock, const struct command *cmd);
size_t rfs_send_answer(const int sock, const struct answer *ans);
size_t rfs_send_data(const int sock, const void *data, const size_t data_len);
size_t rfs_receive_cmd(const int sock, struct command *cmd);
size_t rfs_receive_answer(const int sock, struct answer *ans);
size_t rfs_receive_data(const int sock, void *data, const size_t data_len);

#ifdef RFS_DEBUG
void dump_sendrecv_stats();
#endif

#endif // SEND_H
