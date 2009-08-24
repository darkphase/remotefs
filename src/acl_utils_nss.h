/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

/** utilities for ACL support with NSS */

#include "options.h"

#ifdef ACL_AVAILABLE

#ifndef ACL_UTILS_NSS_H
#define ACL_UTILS_NSS_H

#include "acl_utils.h"

/** makes names like "name@remote_host" from "name" 
and returns its uid/gid (if available) */
uint32_t nss_resolve(uint16_t type, const char *name, size_t name_len, void *instance_casted);

/** returns name of remote user/group as in name@remote_host
don't forget to free() result */
char* nss_reverse_resolve(uint16_t type, uint32_t id, void *instance_casted);

#endif /* ACL_UTILS_NSS_H */

#endif /* ACL_AVAILABLE */

