/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef NSS_PROCESSING_H
#define NSS_PROCESSING_H

#ifdef WITH_UGO

/** nss commands processing */

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct rfs_instance;
struct command;

int process_command(struct rfs_instance *instance, int sock, struct command *cmd);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* WITH_UGO */
#endif /* NSS_PROCESSING_H */
