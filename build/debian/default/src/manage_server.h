/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef RFSNSS_MANAGE_SERVER
#define RFSNSS_MANAGE_SERVER

/** server management routines */

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct config;
struct group_info;
struct rfs_list;
struct user_info;

int add_rfs_server(struct config *config, const char *server_name);

int add_user(struct rfs_list **root, const char *user, uid_t uid);
int add_group(struct rfs_list **root, const char *group, gid_t gid);

struct user_info* find_user_name(const struct rfs_list *root, const char *name);
struct user_info* find_user_uid(const struct rfs_list *root, const uid_t uid);
struct group_info* find_group_name(const struct rfs_list *root, const char *name);
struct group_info* find_group_gid(const struct rfs_list *root, const gid_t gid);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* RFSNSS_MANAGE_SERVER */

