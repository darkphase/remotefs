/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

/** client users/groups lookup routines (using get[pw/gr]* interface) */

#ifndef ID_LOOKUP_CLIENT_H
#define ID_LOOKUP_CLIENT_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct rfs_instance;

/** get uid of named user 
(will default to root's or nobody's uid if user can't be found) */
uid_t lookup_user(const struct rfs_instance *instance, const char *name);

/** get gid of named group
(will try to default group to name looked up in users if name not found in groups) */
gid_t lookup_group(const struct rfs_instance *instance, const char *name, const char *username);

/** get username with specified uid 
(will default to root or nobody if uid can't be found) */
const char* lookup_uid(const struct rfs_instance *instance, uid_t uid);

/** get groupname with specified gid 
(will default to username with uid if group can't be found) */
const char* lookup_gid(const struct rfs_instance *instance, gid_t gid, uid_t uid);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* ID_LOOKUP_CLIENT_H */

