/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef SERVER_HANDLERS_RFS_H
#define SERVER_HANDLERS_RFS_H

/** rfs specific server handlers  */

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#include "rfs/auth.h"
#include "rfs/changepath.h"
#include "rfs/closeconnection.h"
#include "rfs/getexportopts.h"
#include "rfs/handshake.h"
#include "rfs/keepalive.h"
#include "rfs/listexports.h"
#include "rfs/request_salt.h"

#include "../nss/handlers_nss.h"

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* SERVER_HANDLERS_RFS_H */
