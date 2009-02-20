/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef ID_LOOKUP_H
#define ID_LOOKUP_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#include <sys/types.h>

struct list;

/** get uid of named user */
uid_t get_uid(const struct list *uids, const char *name);

/** get username with specified uid */
const char* get_uid_name(const struct list *uids, uid_t uid);

/** get gid of named group */
gid_t get_gid(const struct list *gids, const char *name);

/** get groupname with specified gid */
const char* get_gid_name(const struct list *gids, gid_t gid);

/** create list of uids */
int create_uids_lookup(struct list **uids);

/** create list of gids */
int create_gids_lookup(struct list **gids);

/** destroy list of uids */
void destroy_uids_lookup(struct list **uids);

/** destroy list of gids */
void destroy_gids_lookup(struct list **gids);

/** get uid of named user 
(will default to root's or nobody's uid if user can't be found) */
uid_t lookup_user(const struct list *uids, const char *name);

/** get gid of named group
(will try to default group to name looked up in users if name not found in groups) */
gid_t lookup_group(const struct list *gids, const char *name, const char *username);

/** get username with specified uid 
(will default to root or nobody if uid can't be found) */
const char* lookup_uid(const struct list *uids, uid_t uid);

/** get groupname with specified gid 
(will default to username with uid if group can't be found) */
const char* lookup_gid(const struct list *gids, gid_t gid, const struct list *uids, uid_t uid);

#if defined WITH_NSS
const char *get_user_name(struct list **uid_list, int min);
const char *get_group_name(struct list **uid_list, int min);
void init_nss_host_name(char *name);
#endif

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif // ID_LOOKUP_H
