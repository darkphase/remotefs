/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef PASSWD_H
#define PASSWD_H

/** passwd database routines */

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct list;

/** authentication record */
struct auth_entry
{
	char *user;
	char *passwd;
};

/** supported database operations */
enum operations { OP_CHANGE = 0, OP_DEFAULT = OP_CHANGE, OP_DELETE, OP_HELP, OP_LOCK, OP_UNLOCK, OP_MAX };

/** load records from passwd db */
int load_passwords(const char *passwd_file, struct list **auths);

/** save in-memory records to db */
int save_passwords(const char *passwd_file, const struct list *auths);

/** free allocated memory */
void release_passwords(struct list **auths);

/** add or replace user with specified name */
int add_or_replace_auth(struct list **auths, const char *user, const char *passwd);

/** change user's password */
int change_auth_password(struct list **auths, const char *user, const char *passwd);

/** get password for specified user */
const char *get_auth_password(const struct list *auths, const char *user);

/** delete user's record from db */
int delete_auth(struct list **auths, const char *user);

#ifdef RFS_DEBUG
/** write db to output. debug only */
void dump_passwords(const struct list *auths);
#endif

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* PASSWD_H */

