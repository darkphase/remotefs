/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include "../options.h"

#if defined ACL_AVAILABLE

#ifndef SERVER_HANDLERS_ACL_H
#define SERVER_HANDLERS_ACL_H

/** ACL-specific server handlers */

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#include "handlers/getxattr.h"
#include "handlers/setxattr.h"

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* SERVER_HANDLERS_ACL_H */
#endif /* ACL_AVAILABLE */
