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

int put_to_uids(const char *name, const uid_t uid);
uid_t get_uid(const char *name);
const char* get_uid_name(uid_t uid);

int put_to_gids(const char *name, const gid_t gid);
gid_t get_gid(const char *name);
const char* get_gid_name(gid_t gid);

int create_uids_lookup();
int create_gids_lookup();
void destroy_uids_lookup();
void destroy_gids_lookup();

uid_t lookup_user(const char *name);
/** will try to default group to name looked up in users if name not found in groups */
gid_t lookup_group(const char *name, const char *username);

const char* lookup_uid(uid_t uid);
const char* lookup_gid(gid_t gid, uid_t uid);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif // ID_LOOKUP_H
