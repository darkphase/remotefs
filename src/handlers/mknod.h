/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef SERVER_HANDLERS_MKNOD_H
#define SERVER_HANDLERS_MKNOD_H

/** mknod */

struct command;
struct rfsd_instance;
struct sockaddr_in;

int _handle_mknod(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);

#endif /* SERVER_HANDLERS_MKNOD_H */
