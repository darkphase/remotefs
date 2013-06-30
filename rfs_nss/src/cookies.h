/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef RFSNSS_COOKIES_H
#define RFSNSS_COOKIES_H

/** routines for cookies used in *ent handling */

#include <sys/types.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct config;

struct cookie
{
	pid_t pid;
	unsigned value;
	time_t updated;
};

struct cookie* create_cookie(void **cookies, pid_t pid);
int delete_cookie(void **cookies, pid_t pid);
struct cookie* get_cookie(void **cookies, pid_t pid);

void clear_cookies(void **cookies);
void destroy_cookies(void **cookies);

#ifdef RFS_DEBUG
void dump_cookies(const void *cookies);
void dump_cookie(const struct cookie *cookie);
#endif

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* RFSNSS_COOKIES_H */

