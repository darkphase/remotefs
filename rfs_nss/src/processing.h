/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef RFSNSS_PROCESSING
#define RFSNSS_PROCESSING

/** basic commands processing */

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct config;
struct nss_command;

int process_inc(int sock, const struct nss_command *cmd, struct config *config);
int process_dec(int sock, const struct nss_command *cmd, struct config *config);
int process_addserver(int sock, const struct nss_command *cmd, struct config *config);
int process_adduser(int sock, const struct nss_command *cmd, struct config *config);
int process_addgroup(int sock, const struct nss_command *cmd, struct config *config);

int process_getpwnam(int sock, const struct nss_command *cmd, struct config *config);
int process_getpwuid(int sock, const struct nss_command *cmd, struct config *config);
int process_getgrnam(int sock, const struct nss_command *cmd, struct config *config);
int process_getgrgid(int sock, const struct nss_command *cmd, struct config *config);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* RFSNSS_PROCESSING */

