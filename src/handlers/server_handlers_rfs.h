/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef SERVER_HANDLERS_RFS_H
#define SERVER_HANDLERS_RFS_H

/** rfs specific server handlers  */

#include "../options.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#include "rfs/listexports.h"

struct command;
struct rfsd_instance;
struct sockaddr_in;

int _handle_request_salt(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int _handle_auth(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int _handle_closeconnection(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int _handle_changepath(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int _handle_keepalive(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int _handle_getexportopts(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);

#include "../nss/server_handlers_nss.h"
#include "../ssl/server_handlers_ssl.h"

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* SERVER_HANDLERS_RFS_H */
