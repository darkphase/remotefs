/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_H
#define OPERATIONS_H

/** not-safe operations */

/* if connection lost after executing the operation
and operation failed, then try it one more time 

pass ret (int) as return value of this macro */
#define PARTIALY_DECORATE(ret, func, instance, ...)                 \
	if(check_connection((instance)) == 0)                       \
	{                                                           \
		(ret) = (func)((instance), ## __VA_ARGS__);         \
		if ((ret) == -ECONNABORTED                          \
		&& check_connection((instance)) == 0)               \
		{                                                   \
			(ret) = (func)((instance), ## __VA_ARGS__); \
		}                                                   \
	}

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#include "chmod.h"
#include "chown.h"
#include "create.h"
#include "getattr.h"
#include "flush.h"
#include "link.h"
#include "lock.h"
#include "mkdir.h"
#include "mknod.h"
#include "open.h"
#include "read.h"
#include "readdir.h"
#include "readlink.h"
#include "rename.h"
#include "release.h"
#include "rmdir.h"
#include "statfs.h"
#include "symlink.h"
#include "truncate.h"
#include "write.h"
#include "unlink.h"
#include "utime.h"
#include "utimens.h"

#include "../acl/operations_acl.h"

#include "operations_rfs.h"

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* OPERATIONS_H */
