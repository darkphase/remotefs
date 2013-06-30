/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef RFSNSS_PROCESSING_ENT_H
#define RFSNSS_PROCESSING_ENT_H

/** *ent commands processing */

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct config;
struct nss_command;

int process_getpwent(int sock, const struct nss_command *cmd, struct config *config);
int process_setpwent(int sock, const struct nss_command *cmd, struct config *config);
int process_endpwent(int sock, const struct nss_command *cmd, struct config *config);

int process_getgrent(int sock, const struct nss_command *cmd, struct config *config);
int process_setgrent(int sock, const struct nss_command *cmd, struct config *config);
int process_endgrent(int sock, const struct nss_command *cmd, struct config *config);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* RFSNSS_PROCESSING_ENT_H */

