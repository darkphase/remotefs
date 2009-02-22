/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef NSS_CLIENT_H
#define NSS_CLIENT_H

/** nss client */

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

int nss_check_user(const char *full_name);
int nss_check_group(const char *full_name);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* NSS_CLIENT_H */

