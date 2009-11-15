/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

/* Linux specific ACL routines */

#include "../options.h"

#if (defined LINUX && defined ACL_AVAILABLE)

#ifndef ACL_LINUX_H
#define ACL_LINUX_H

#include <stdint.h>
#include <sys/acl.h>

/** xattr's size of ACL record 
\param recs_number optional calculated number of ACL entries */
size_t xattr_acl_size(const acl_t acl, size_t *recs_number);

/** convert ACL record to xattr value */
int rfs_acl_to_xattr(const acl_t acl, void *xattr, size_t size);

/** covert xattr value to ACL record 
don't forget to acl_free() result */
int rfs_acl_from_xattr(const void *xattr, size_t size, acl_t *acl);

#endif /* ACL_LINUX_H */
#endif /* ACL_AVAILABLE && LINUX */

