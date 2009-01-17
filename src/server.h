/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef SERVER_H
#define SERVER_H

/** server routines */

#include <arpa/inet.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct rfsd_instance;
struct command;

/** reject client's request */
int reject_request(struct rfsd_instance *instance, const struct command *cmd, int32_t ret_errno);

/** reject client's request with cleaning socket from incoming data */
int reject_request_with_cleanup(struct rfsd_instance *instance, const struct command *cmd, int32_t ret_errno);

int handle_command(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);

#ifndef WITH_IPV6
int handle_connection(struct rfsd_instance *instance, int client_socket, const struct sockaddr_in *client_addr);
#else
int handle_connection(struct rfsd_instance *instance, int client_socket, const struct sockaddr_storage *client_addr);
#endif

/** close connection */
void server_close_connection(struct rfsd_instance *instance);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* SERVER_H */
