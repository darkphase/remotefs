/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef RFSNSS_CLIENT_ENT_H
#define RFSNSS_CLIENT_ENT_H

/** *ent client functions */

#include <sys/types.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct passwd;
struct group;

struct passwd* rfsnss_getpwent(void);
void rfsnss_setpwent(void);
void rfsnss_endpwent(void);

struct group* rfsnss_getgrent(void);
void rfsnss_setgrent(void);
void rfsnss_endgrent(void);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* RFSNSS_CLIENT_ENT_H */

