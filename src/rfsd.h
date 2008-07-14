#ifndef SERVER_H
#define SERVER_H

/* server interface, basically for signals */

#include <stdint.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct command;

int add_file_to_open_list(int file);
int remove_file_from_open_list(int file);
void server_close_connection(int socket);
void stop_server();
void check_keep_alive();
int reject_request(const int client_socket, const struct command *cmd, int32_t ret_errno);
int reject_request_with_cleanup(const int client_socket, struct command *cmd, int32_t ret_errno);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif // SERVER_H
