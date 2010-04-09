/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include "../../options.h"

#if defined ACL_AVAILABLE

#ifndef SERVER_HANDLERS_ACL_GEXATTR_H
#define SERVER_HANDLERS_ACL_GEXATTR_H

/** getxattr */

struct command;
struct rfsd_instance;
struct sockaddr_in;

int _handle_getxattr(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);

#endif /* SERVER_HANDLERS_ACL_GETXATTR_H */
#endif /* ACL_AVAILABLE */