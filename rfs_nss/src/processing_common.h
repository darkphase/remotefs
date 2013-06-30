/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef RFSNSS_PROCESSING_COMMON_H
#define RFSNSS_PROCESSING_COMMON_H

/** common routines for commands processing */

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct nss_answer;

int send_answer(int sock, const struct nss_answer *ans, const char *data);
int recv_command_data(int sock, const struct nss_command *cmd, char **data);

int reject_request(int sock, const struct nss_command *cmd, int ret_errno);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* RFSNSS_PROCESSING_COMMON_H */

