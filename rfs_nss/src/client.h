/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef RFSNSS_CLIENT_H
#define RFSNSS_CLIENT_H

/** basic NSS functions */

#include <sys/types.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct passwd* rfsnss_getpwnam(const char *name);
struct passwd* rfsnss_getpwuid(uid_t uid);
struct group* rfsnss_getgrnam(const char *name);
struct group* rfsnss_getgrgid(gid_t gid);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* RFSNSS_CLIENT_H */

