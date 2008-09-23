/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <string.h>
#include <errno.h>
#include <termios.h>

#include "config.h"
#include "passwd.h"
#include "crypt.h"
#include "signals.h"

static char *login = NULL;
enum operations operation = OP_DEFAULT;
static struct termios stored_settings = { 0 };
static unsigned need_to_restore_termio = 0;

/* forward declarations */
static int change_password(const char *login);
static int delete_password(const char *login);
static int lock_password(const char *login);
static int unlock_password(const char *login);

static void usage(const char *app_name)
{
	printf("usage: %s [options] [LOGIN]\n"
	"\n"
	"Options:\n"
	"-d\t\tdelete the password for the named account\n"
	"-h\t\tdisplay this help message and exit\n"
	"-l\t\tlock the named account\n"
	"-u\t\tunlock the named account\n"
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
	while ((opt = getopt(argc, argv, "dhlu")) != -1)
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
	
	int load_ret = load_passwords();
	if (load_ret != 0)
	{
		ERROR("Error loading passwords: %s\n", strerror(-load_ret));
		exit(1);
	}
	
	dump_passwords();
	
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
		int save_ret = save_passwords();
		if (save_ret != 0)
		{
			ERROR("Error saving passwords: %s\n", strerror(-save_ret));
			exit(1);
		}
	}
	
	release_passwords();
	
	return ret;
}

int change_password(const char *login)
{
	char password1[32] = { 0 };
	char password2[32] = { 0 };
	
	printf("Changing password for %s.\n", login);
	printf("New password (%ud characters max): ", sizeof(password1));
	
	tcgetattr(0, &stored_settings);
	struct termios new_settings = stored_settings;
	
	new_settings.c_lflag &= (~ECHO);
	
	need_to_restore_termio = 1;
	
	tcsetattr(0, TCSANOW, &new_settings);
	
	fgets(password1, sizeof(password1) - 1, stdin);

	tcsetattr(0, TCSANOW, &stored_settings);
	printf("\n");
	printf("Repeat password: ");
	tcsetattr(0, TCSANOW, &new_settings);
	
	fgets(password2, sizeof(password2) - 1, stdin);
	
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
	
	if (add_or_replace_auth(login, passwd) != 0)
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
	const char *auth_passwd = get_auth_password(login);
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
	
	if (change_auth_password(login, passwd) != 0)
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
	const char *auth_passwd = get_auth_password(login);
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
	
	if (change_auth_password(login, auth_passwd + 1) != 0)
	{
		ERROR("%s\n", "Specified account isn't exist");
		
		return 1;
	}
	
	INFO("Account \"%s\" is unlocked\n", login);
	
	return 0;
}

int delete_password(const char *login)
{
	const char *auth_passwd = get_auth_password(login);
	if (auth_passwd == NULL)
	{
		ERROR("%s\n", "Specified account isn't exist");
		return 1;
	}
	
	if (delete_auth(login) != 0)
	{
		ERROR("%s\n", "Specified account isn't exist");
		
		return 1;
	}
	
	return 0;
}
