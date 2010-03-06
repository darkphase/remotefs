/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef SERVER_HANDLERS_UTIME_H
#define SERVER_HANDLERS_UTIME_H

/** utime */

#include <stdint.h>
#include <sys/types.h>

struct command;
struct rfsd_instance;
struct sockaddr_in;

int _process_utime(struct rfsd_instance *instance, const struct command *cmd, const char *path, unsigned is_null, uint64_t actime, uint64_t modtime);
int _handle_utime(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);

#endif /* SERVER_HANDLERS_UTIME_H */
