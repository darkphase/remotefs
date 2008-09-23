/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef SERVER_H
#define SERVER_H

/** server interface, basically for signals */

#include <stdint.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct command;
extern struct rfsd_config rfsd_config;
extern unsigned char directory_mounted;
extern struct rfs_export *mounted_export;

int add_file_to_open_list(int file);
int remove_file_from_open_list(int file);
void server_close_connection(int socket);
void stop_server(void);
void check_keep_alive(void);
int reject_request(const int client_socket, const struct command *cmd, int32_t ret_errno);
int reject_request_with_cleanup(const int client_socket, const struct command *cmd, int32_t ret_errno);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif // SERVER_H
