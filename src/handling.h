/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef HANDLING_H
#define HANDLING_H

/** server commands handling routines and utils */

#include <stdint.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct rfs_command;
struct rfsd_instance;
struct sockaddr_in;

/** reject client's request */
int reject_request(struct rfsd_instance *instance, const struct rfs_command *cmd, int32_t ret_errno);

/** reject client's request with cleaning socket from incoming data */
int reject_request_with_cleanup(struct rfsd_instance *instance, const struct rfs_command *cmd, int32_t ret_errno);

/** handle received command */
int handle_command(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct rfs_command *cmd);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* HANDLING_H */
