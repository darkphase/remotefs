/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef SERVER_H
#define SERVER_H

/** server routines */

#include <stdint.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct command;
struct rfsd_instance;
struct sockaddr_in;

/** reject client's request */
int reject_request(struct rfsd_instance *instance, const struct command *cmd, int32_t ret_errno);

/** reject client's request with cleaning socket from incoming data */
int reject_request_with_cleanup(struct rfsd_instance *instance, const struct command *cmd, int32_t ret_errno);

/** start listening and accepting connections */
int start_server(struct rfsd_instance *instance, unsigned daemonize, unsigned force_ipv4, unsigned force_ipv6);

/** close connection, release server and exit */
void stop_server(struct rfsd_instance *instance);

/** close connection */
void server_close_connection(struct rfsd_instance *instance);

/** release resource allocated by server
including pidfile, exports, passwords and etc */
void release_server(struct rfsd_instance *instance);

/** check if keep-alive has expired */
void check_keep_alive(struct rfsd_instance *instance);

/** handle received command */
int handle_command(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* SERVER_H */

