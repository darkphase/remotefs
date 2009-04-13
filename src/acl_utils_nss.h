/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

/** utilities for ACL support with NSS */

#ifdef WITH_ACL

#ifndef ACL_UTILS_NSS_H
#define ACL_UTILS_NSS_H

#include "acl_utils.h"

struct rfs_instance;

unsigned acl_need_nss_patching(const char *acl_text);

/** don't forget to free() result */
char* patch_acl_for_server(const char *acl_text, const struct rfs_instance *instance);

int patch_acl_from_server(rfs_acl_t *acl, int count, const struct rfs_instance *instance);

#endif /* ACL_UTILS_NSS_H */

#endif /* WITH_ACL */

