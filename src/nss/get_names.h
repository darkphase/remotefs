/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include "../options.h"

#ifdef RFSNSS_AVAILABLE

#ifndef OPERATIONS_NSS_GETNAMES_H
#define OPERATIONS_NSS_GETNAMES_H

struct rfs_instance;
int rfs_getnames(struct rfs_instance *instance);

#endif /* OPERATIONS_NSS_GETNAMES_H */
#endif /* RFNSS_AVAILABLE */
