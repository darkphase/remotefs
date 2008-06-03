#ifndef PASSWD_H
#define PASSWD_H

struct auth_entry
{
	char *user;
	char *passwd;
};

enum operations { OP_CHANGE = 0, OP_DEFAULT = OP_CHANGE, OP_DELETE, OP_HELP, OP_LOCK, OP_UNLOCK, OP_MAX };

int load_passwords();
int save_passwords();
void release_passwords();
int add_or_replace_auth(const char *user, const char *passwd);
int change_auth_password(const char *user, const char *passwd);
const char *get_auth_password(const char *user);
int delete_auth(const char *user);

void dump_passwords();

#endif // PASSWD_H
