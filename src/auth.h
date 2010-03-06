/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef AUTH_H
#define AUTH_H

/** auth utils */

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct rfsd_instance;
struct rfs_export;

int check_password(struct rfsd_instance *instance);
int check_permissions(struct rfsd_instance *instance, const struct rfs_export *export_info, const char *client_ip_addr);
int generate_salt(char *salt, size_t max_size);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* AUTH_H */
