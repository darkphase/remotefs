/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "config.h"
#include "crypt.h"
#include "passwd.h"
#include "signals.h"

static char *login = NULL;
static enum operations operation = OP_DEFAULT;
static struct termios stored_settings = { 0 };
static unsigned need_to_restore_termio = 0;
static struct list *auths = NULL;

static char *passwd_file = NULL;

/* forward declarations */
int change_password(const char *login);
int delete_password(const char *login);
int lock_password(const char *login);
int unlock_password(const char *login);

static void usage(const char *app_name)
{
	printf("usage: %s [options] [LOGIN]\n"
	"\n"
	"Options:\n"
	"-d\t\tdelete the named account\n"
	"-h\t\tdisplay this help message and exit\n"
	"-l\t\tlock the named account\n"
	"-u\t\tunlock the named account\n"
	"-s [path]\tpath to passwd file\n"
	"\n"
	, app_name);
}

static int check_opts()
{
	if (login != NULL 
	&& strchr(login, ':') != NULL)
	{
		ERROR("%s\n", "Username can not contain ':'");
		return -1;
	}
	
	return 0;
}

static int parse_opts(int argc, char **argv)
{
	int opt;
	while ((opt = getopt(argc, argv, "dhlus:")) != -1)
	{
		if (operation != OP_DEFAULT)
		{
			return -1;
		}
				
		switch (opt)
		{	
			case 'd':
				operation = OP_DELETE;
				break;
			case 'h':
				usage(argv[0]);
				exit(0);
				break;
			case 'l':
				operation = OP_LOCK;
				break;
			case 'u':
				operation = OP_UNLOCK;
				break;
			case 's':
				free(passwd_file);
				passwd_file = strdup(optarg);
				break;
			default:
				return -1;
		}
	}

	if (optind >= argc)
	{
		struct passwd *pwuid = NULL;
		if ((pwuid = getpwuid(getuid())) == NULL)
		{
			return -1;
		}
		
		login = strdup(pwuid->pw_name);
	}
	else
	{
		login = strdup(argv[optind]);
	}
	
	return 0;
}

static void signal_handler_passwd(int signal, siginfo_t *sig_info, void *ucontext_t_casted)
{
	switch (signal)
	{	
	case SIGTERM:
	case SIGPIPE:
	case SIGABRT:
	case SIGINT:
	case SIGQUIT:
		if (need_to_restore_termio != 0)
		{
			tcsetattr(0, TCSANOW, &stored_settings);
			printf("\n");
		}
		exit(0);
	}
}

static void install_signal_handlers()
{
	install_signal_handler(SIGTERM, signal_handler_passwd);
	install_signal_handler(SIGPIPE, signal_handler_passwd);
	install_signal_handler(SIGABRT, signal_handler_passwd);
	install_signal_handler(SIGINT, signal_handler_passwd);
	install_signal_handler(SIGQUIT, signal_handler_passwd);
}

int main(int argc, char **argv)
{
	passwd_file = strdup(DEFAULT_PASSWD_FILE);

	if (parse_opts(argc, argv) != 0)
	{
		usage(argv[0]);
		exit(1);
	}
	
	if (check_opts() != 0)
	{
		exit(1);
	}
	
	if (operation >= OP_MAX
	|| login == NULL)
	{
		usage(argv[0]);
		exit(1);
	}
	
	int load_ret = load_passwords(passwd_file, &auths);
	if (load_ret != 0)
	{
		ERROR("Error loading passwords: %s\n", strerror(-load_ret));
		exit(1);
	}
	
	install_signal_handlers();
	
	int ret = 0;
	switch (operation)
	{
		case OP_HELP:
			usage(argv[0]);
			exit(0);
			break;
			
		case OP_CHANGE:
			ret = change_password(login);
			break;
		
		case OP_DELETE:
			ret = delete_password(login);
			break;
		
		case OP_LOCK:
			ret = lock_password(login);
			break;
			
		case OP_UNLOCK:
			ret = unlock_password(login);
			break;
		
		default:
			ret = 1;
	}

	free(login);
	
	if (ret == 0)
	{
		int save_ret = save_passwords(passwd_file, auths);
		if (save_ret != 0)
		{
			ERROR("Error saving passwords: %s\n", strerror(-save_ret));
			free(passwd_file);
			exit(1);
		}
	}
	
#ifdef RFS_DEBUG
	dump_passwords(auths);
#endif
	
	release_passwords(&auths);
	free(passwd_file);
	
	return ret;
}

int change_password(const char *login)
{
	char password1[32] = { 0 };
	char password2[32] = { 0 };
	
	printf("Changing password for %s.\n", login);
	printf("New password (%u characters max): ", (unsigned int)sizeof(password1));
	
	tcgetattr(0, &stored_settings);
	struct termios new_settings = stored_settings;
	
	new_settings.c_lflag &= (~ECHO);
	
	need_to_restore_termio = 1;
	
	tcsetattr(0, TCSANOW, &new_settings);
	
	if (fgets(password1, sizeof(password1) - 1, stdin) == NULL)
	{
		memset(password1, 0, sizeof(password1));
		return -1;
	}

	tcsetattr(0, TCSANOW, &stored_settings);
	printf("\n");
	printf("Repeat password: ");
	tcsetattr(0, TCSANOW, &new_settings);
	
	if (fgets(password2, sizeof(password2) - 1, stdin) == NULL)
	{
		memset(password1, 0, sizeof(password1));
		memset(password2, 0, sizeof(password2));
		return -1;
	}
	
	tcsetattr(0, TCSANOW, &stored_settings);
	printf("\n");
	
	need_to_restore_termio = 0;
	
	if (strcmp(password1, password2) != 0)
	{
		memset(password1, 0, sizeof(password1));
		memset(password2, 0, sizeof(password2));
		ERROR("%s\n", "Passwords doesn't match");
		
		return -1;
	}
	
	memset(password2, 0, sizeof(password2));
	
	if (strlen(password1) > 1 && password1[strlen(password1) - 1] == '\n')
	{
		password1[strlen(password1) - 1] = '\0';
	}
	
	char *passwd = passwd_hash(password1, EMPTY_SALT);
	memset(password1, 0, sizeof(password1));
	
	if (add_or_replace_auth(&auths, login, passwd) != 0)
	{
		ERROR("%s\n", "Error adding item to passwords list");
		
		free(passwd);
		return -1;
	}
	
	free(passwd);
	
	INFO("Password changed for %s\n", login);
	
	return 0;
}

int lock_password(const char *login)
{
	const char *auth_passwd = get_auth_password(auths, login);
	if (auth_passwd == NULL)
	{
		ERROR("%s\n", "Specified account isn't exist");
		return 1;
	}
	
	if (strlen(auth_passwd) < 1)
	{
		ERROR("%s\n", "Password is empty");
		return 1;
	}
	
	if (auth_passwd[0] == '*')
	{
		ERROR("%s\n", "Account is already locked");
		return 1;
	}
	
	char *passwd = malloc(strlen(auth_passwd) + 2);
	memset(passwd, 0, strlen(auth_passwd) + 2);
	passwd[0] = '*';
	strcat(passwd, auth_passwd);
	
	if (change_auth_password(&auths, login, passwd) != 0)
	{
		ERROR("%s\n", "Specified account isn't exist");
		
		free(passwd);
		return 1;
	}
	
	free(passwd);
	
	INFO("Account \"%s\" is locked\n", login);
	
	return 0;
}

int unlock_password(const char *login)
{
	const char *auth_passwd = get_auth_password(auths, login);
	if (auth_passwd == NULL)
	{
		ERROR("%s\n", "Specified account isn't exist");
		return 1;
	}
	
	if (strlen(auth_passwd) < 1)
	{
		ERROR("%s\n", "Password is empty");
		return 1;
	}
	
	if (auth_passwd[0] != '*')
	{
		ERROR("%s\n", "Account isn't locked");
		return 1;
	}
	
	if (change_auth_password(&auths, login, auth_passwd + 1) != 0)
	{
		ERROR("%s\n", "Specified account isn't exist");
		
		return 1;
	}
	
	INFO("Account \"%s\" is unlocked\n", login);
	
	return 0;
}

int delete_password(const char *login)
{
	const char *auth_passwd = get_auth_password(auths, login);
	if (auth_passwd == NULL)
	{
		ERROR("%s\n", "Specified account isn't exist");
		return 1;
	}
	
	if (delete_auth(&auths, login) != 0)
	{
		ERROR("%s\n", "Specified account isn't exist");
		
		return 1;
	}
	
	return 0;
}
