/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef RFSNSS_CLIENT_COMMON_H
#define RFSNSS_CLIENT_COMMON_H

/** function common for client and client for server */

#include <sys/types.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct nss_command;
struct nss_answer;

/** connect to rfs_nss server
\param uid pass (uid_t)-1 to (try to) connect to shared server */
int make_connection(uid_t uid);

/** close connection to rfs_nss server */
int close_connection(int sock);

/** send command with optional data */
int send_command(int sock, const struct nss_command *cmd, const char *data);

/** receive answer with optional data 
\param data will be allocated if needed. do not forget to free() it */
int recv_answer(int sock, struct nss_answer *ans, char **data);

/** check if rfs_nss server for this uid is already running 
\param uid pass (uid_t)-1 for shared server */
unsigned rfsnss_is_server_running(uid_t uid);

/** macro to make use of private and shared servers */
#define CHECK_BOTH_SERVERS(func, ...)                                                 \
	void *ret = func(getuid(), ## __VA_ARGS__);    /* use private server first */ \
	if (ret != NULL) { return ret; }         /* if no result */                   \
	return func((uid_t)(-1), ## __VA_ARGS__)      /* try shared server */         \

#define CHECK_BOTH_SERVERS_VOID(func)                                                 \
	void *ret = (func)(getuid());            /* use private server first */       \
	if (ret != NULL) { return ret; }         /* if no result */                   \
	return (func)((uid_t)(-1))              /* try shared server */               \

#define APPLY_TO_BOTH_SERVERS(func)                                                   \
	(func)(getuid());                                                             \
	(func)((uid_t)-1)                                                             \

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* RFSNSS_CLIENT_COMMON_H */

