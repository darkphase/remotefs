/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#if ! defined _RFS_NSSLIB_H_
#define _RFS_NSSLIB_H_

#include <pwd.h>
#include <grp.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#define LIBRFS_NSS "librfs_nss.so"

/************************************************
 *
 * The following functions allow to add am user
 * or group name to the rfs_nss server.
 *
 * The parameter char *name is madatory.
 * The parameter char *host is optional.
 *
 * These fuction return the uid/gid for the
 * added user/group or 0 if this was not possible.
 *
 * The name entered to the rfs_nss databases are
 * name if host is null or name@host if host
 * contain the host name or ip address.
 *
 ***********************************************/
uid_t rfs_putpwnam(char *name, char *host);
gid_t rfs_putgrnam(char *name, char *host);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif
