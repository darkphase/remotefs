/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef NAMES_H
#define NAMES_H

/** NSS-names specific routines */

#include <sys/types.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct rfs_instance;

/** don't forget to free() result */
char* extract_name(const char *full_name);

/** don't forget to free() result */
char* extract_server(const char *full_name);

unsigned is_nss_name(const char *name);

/** don't forget to free() result */
char* local_nss_name(const char *full_name, const struct rfs_instance *instance);

/** don't forget to free() result */
char* remote_nss_name(const char *short_name, const struct rfs_instance *instance);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* NAMES_H */
