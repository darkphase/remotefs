/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include "../options.h"

#ifdef ACL_OPERATIONS_AVAILABLE

#ifndef OPERATIONS_ACL_H
#define OPERATIONS_ACL_H

/** ACL specific rfs operations */

#include <sys/types.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#include "getxattr.h"
#include "setxattr.h"

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* OPERATIONS_ACL_H */
#endif /* ACL_OPERATIONS_AVAILABLE */
