/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

/** resolving of names using rfs_nss */

#include "../options.h"

#ifdef ACL_OPERATIONS_AVAILABLE

#ifndef ACL_NSS_RESOLVE_H
#define ACL_NSS_RESOLVE_H

#include <stdint.h>
#include <sys/acl.h>

/** makes names like "name@remote_host" from "name" 
and returns its uid/gid (if available) */
uint32_t nss_resolve(acl_tag_t tag, const char *name, size_t name_len, void *instance_casted);

/** returns name of remote user/group as in name@remote_host
don't forget to free() result */
char* nss_reverse_resolve(acl_tag_t tag, void *id, void *instance_casted);

#endif /* ACL_NSS_RESOLVE_H */

#endif /* ACL_OPERATIONS_AVAILABLE */

