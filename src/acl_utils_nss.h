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

struct id_lookup_info;

struct resolve_params
{
	const char *host;
	const struct id_lookup_info *lookup;
};

uint32_t nss_resolve(uint16_t type, const char *name, size_t name_len, void *params_casted);
char* nss_reverse_resolve(uint16_t type, uint32_t id, void *params_casted);

#endif /* ACL_UTILS_NSS_H */

#endif /* WITH_ACL */

