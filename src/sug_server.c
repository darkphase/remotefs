/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"
#include "sug_server.h"
#include "passwd.h"
#include "exports.h"
#include "utils.h"
#include "instance.h"

static int check_listen_address(const char *address)
{
	/* resolving won't happen wor listen address,
	so this is always an ip-address */
	
	if (
#ifdef WITH_IPV6
	is_ipv6_local(address) == 0 &&
#endif
	is_ipv4_local(address) == 0)
	{
		WARN("%s\n", "WARNING: You are listening on interface outside of private network.");
		return -1;
	}
	
	return 0;
}

static int check_passwd(struct list *auths)
{
	if (get_auth_password(auths, "root") != NULL)
	{
		WARN("%s\n", "WARNING: Passwd record for root found. You shouldn't do that (`rfspasswd -d root`)");
		return -1;
	}
	
	return 0;
}

static int check_modes(const char *exports_file, const char *passwd_file)
{
	int ret = 0;
	
	struct stat exports_stbuf = { 0 };
	/* see comments for the similar code in sug_client.c */
	if (stat(exports_file, &exports_stbuf) == 0)
	{
		if ((unsigned)(exports_stbuf.st_mode & S_IRWXG) != 0
		|| (unsigned)(exports_stbuf.st_mode & S_IRWXO) != 0
		|| exports_stbuf.st_uid != getuid())
		{
			WARN("WARNING: Exports file should be readable/writeable only by user which is running rfsd (`chmod 600 %s`)\n", exports_file);
			ret = -1;
		}
	}
	
	struct stat passwd_stbuf = { 0 };
	if (stat(passwd_file, &passwd_stbuf) == 0)
	{
		if ((unsigned)(passwd_stbuf.st_mode & S_IRWXG) != 0
		|| (unsigned)(passwd_stbuf.st_mode & S_IRWXO) != 0
		|| passwd_stbuf.st_uid != getuid())
		{
			WARN("WARNING: Passwords file should be readable/writeable only by user which is running rfsd (`chmod 600 %s`)\n", passwd_file);
			ret = -1;
		}
	}
	
	return ret;}

int suggest_server(const struct rfsd_instance *instance)
{
	int ret = 0;
	
	if (check_listen_address(instance->config.listen_address) != 0)
	{
		ret = -1;
	}
	
	if (check_passwd(instance->passwd.auths) != 0)
	{
		ret = -1;
	}
	
	if (check_modes(instance->config.exports_file, instance->config.passwd_file) != 0)
	{
		ret = -1;
	}
	
	if (ret != 0)
	{
		INFO("%s\n", "INFO: Please consider SECURITY NOTES section of rfsd man page (`man rfsd`)");
	}
	
	return ret;
}
