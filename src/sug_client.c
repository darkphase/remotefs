/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#include "config.h"
#include "sug_client.h"
#include "utils.h"
#include "instance.h"

#ifdef WITH_SSL
static void check_ssl(const char *host)
{
	char *check_host = strdup(host);
	
	DEBUG("checking SSL against host: %s\n", check_host);
	if (is_ipaddr(check_host) == 0)
	{
		free(check_host);
#ifdef WITH_IPV6
		check_host = resolve_ipv6(check_host);
#else
		check_host = resolve_ipv4(check_host);
#endif
	}
	
	if (check_host == NULL)
	{
		return; /* don't make check on error */
	}
	
	if (
#ifdef WITH_IPV6
	is_ipv6_local(check_host) == 0 &&
#endif
	is_ipv4_local(check_host) == 0)
	{
		WARN("%s\n", "WARNING: Looks like you have specified address outside of private network, but SSL isn't enabled (`rfs ... -o ssl`)");
		WARN("%s\n", "WARNING: BTW, check out SECURITY NOTES section in rfs man page (`man rfs`) for default security policy of remotefs");
	}
	
	free(check_host);
}
#endif

static void check_passwd_file(const char *path)
{
	struct stat stbuf = { 0 };
	if (stat(path, &stbuf) != 0)
	{
		/* yep, this is error
		but it should be handled somewhere else 
		so return silently */
		return;
	}
	
	if ((unsigned)(stbuf.st_mode & S_IRWXG) != 0
	|| (unsigned)(stbuf.st_mode & S_IRWXO) != 0)
	{
		WARN("WARNING: for security reasons you should change mode of your password file to readable/writeable by owner only (`chmod 600 %s`)\n", path);
	}
}

void suggest_client(const struct rfs_instance *instance)
{
#ifdef WITH_SSL
	if (instance->config.enable_ssl == 0)
	{
		check_ssl(instance->config.host);
	}
#endif
	if (instance->config.auth_user != NULL 
	&& instance->config.auth_passwd_file != NULL)
	{
		check_passwd_file(instance->config.auth_passwd_file);
	}
}
