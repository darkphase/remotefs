/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef SERVER_HANDLERS_H
#define SERVER_HANDLERS_H

/** server handlers of rfs operations */

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#include "chmod.h"
#include "chown.h"
#include "create.h"
#include "getattr.h"
#include "link.h"
#include "lock.h"
#include "mkdir.h"
#include "mknod.h"
#include "open.h"
#include "read.h"
#include "readdir.h"
#include "readlink.h"
#include "release.h"
#include "rename.h"
#include "rmdir.h"
#include "statfs.h"
#include "symlink.h"
#include "truncate.h"
#include "write.h"
#include "unlink.h"
#include "utime.h"
#include "utimens.h"

#include "../acl/server_handlers_acl.h"

#include "server_handlers_rfs.h"

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* SERVER_HANDLERS_H */
