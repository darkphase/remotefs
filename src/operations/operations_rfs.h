/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_RFS_H
#define OPERATIONS_RFS_H

/** rfs internal operations */

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#include "rfs/auth.h"
#include "rfs/init.h"
#include "rfs/reconnect.h"
#include "rfs/disconnect.h"
#include "rfs/destroy.h"
#include "rfs/getexportopts.h"
#include "rfs/keepalive.h"
#include "rfs/listexports.h"
#include "rfs/mount.h"
#include "rfs/requestsalt.h"

#include "../nss/operations_nss.h"
#include "../ssl/operations_ssl.h"

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* OPERATIONS_RFS_H */
