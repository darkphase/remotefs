#ifndef PASSWD_H
#define PASSWD_H

/* passwd database routines */

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/** authentication record */
struct auth_entry
{
	char *user;
	char *passwd;
};

/** supported database operations */
enum operations { OP_CHANGE = 0, OP_DEFAULT = OP_CHANGE, OP_DELETE, OP_HELP, OP_LOCK, OP_UNLOCK, OP_MAX };

/** load records from passwd db */
int load_passwords();

/** save in-memory records to db */
int save_passwords();

/** free allocated memory */
void release_passwords();

/** add or replace user with specified name */
int add_or_replace_auth(const char *user, const char *passwd);

/** change user's password */
int change_auth_password(const char *user, const char *passwd);

/** get password for specified user */
const char *get_auth_password(const char *user);

/** delete user's record from db */
int delete_auth(const char *user);

/** write db to output. debug only */
void dump_passwords();

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif // PASSWD_H
