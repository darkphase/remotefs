/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef RFSNSS_CLIENT_FOR_SERVER_H
#define RFSNSS_CLIENT_FOR_SERVER_H

/** client for server */

#include <sys/types.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

int rfsnss_dec(uid_t uid);
int rfsnss_addserver(uid_t uid, const char *rfs_server);
int rfsnss_adduser(const char *username, uid_t user_uid, uid_t uid);
int rfsnss_addgroup(const char *groupname, gid_t group_gid, uid_t uid);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* RFSNSS_CLIENT_FOR_SERVER_H */

