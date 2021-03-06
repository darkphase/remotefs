/*
remotefs file system
See the file AUTHORS for copyright information.

This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

/** server users/groups lookup routines (using id lookup lists) */

#ifndef ID_LOOKUP_H
#define ID_LOOKUP_H

#include <sys/types.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct rfs_list;

struct rfs_id_look_ent
{
	char *name;
	uint64_t id;
};

/** get uid of named user */
uid_t get_uid(const struct rfs_list *uids, const char *name);

/** get username with specified uid */
const char* get_uid_name(const struct rfs_list *uids, uid_t uid);

/** get gid of named group */
gid_t get_gid(const struct rfs_list *gids, const char *name);

/** get groupname with specified gid */
const char* get_gid_name(const struct rfs_list *gids, gid_t gid);

/** create list of uids */
int create_uids_lookup(struct rfs_list **uids);

/** create list of gids */
int create_gids_lookup(struct rfs_list **gids);

/** destroy list of uids */
void destroy_uids_lookup(struct rfs_list **uids);

/** destroy list of gids */
void destroy_gids_lookup(struct rfs_list **gids);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* ID_LOOKUP_H */
