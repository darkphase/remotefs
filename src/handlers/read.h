/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef SERVER_HANDLERS_READ_H
#define SERVER_HANDLERS_READ_H

/** read */

struct rfs_command;
struct rfsd_instance;
struct sockaddr_in;

int _handle_read(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct rfs_command *cmd);

#endif /* SERVER_HANDLERS_H */
