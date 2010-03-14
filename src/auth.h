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

/** \return 0 if user (stored in instance) or client_ip_addr is allowed to access specified export */
int check_permissions(struct rfsd_instance *instance, const struct rfs_export *export_info, const char *client_ip_addr);

/** generate salt for authentication
\param salt generated salt
\param max_size including ending \0, f.i. salt with 4 characters should provide buffer of size 5 
\return 0 on success */
int generate_salt(char *salt, size_t max_size);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* AUTH_H */
