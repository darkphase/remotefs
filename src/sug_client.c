/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <arpa/inet.h>
#if defined FREEBSD
# include <sys/socket.h>
# include <netinet/in.h>
#endif
#include <sys/stat.h>

#include "config.h"
#include "instance_client.h"
#include "ssl/client.h"
#include "sug_common.h"
#include "sug_client.h"
#include "utils.h"

#ifdef WITH_SSL
static void check_protected_connection(const char *host)
{
	int addr_family = AF_UNSPEC;
	char *real_host = host_ip(host, &addr_family);
	
	DEBUG("checking SSL against host: %s\n", real_host);
	
	if (real_host == NULL)
	{
		return; /* don't make check on resolving error */
	}
	
	unsigned is_local_addr = 0;
	
	switch (addr_family)
	{
	case AF_INET:
		is_local_addr = is_ipv4_local(real_host);
		break;
#ifdef WITH_IPV6
	case AF_INET6:
		is_local_addr = is_ipv6_local(real_host);
		break;
#endif
	default:
		free(real_host);
		return; /* don't make check on resolving error */
	}
	
	free(real_host);
	
	if (is_local_addr == 0)
	{
		WARN("%s\n", "WARNING: Looks like you have specified address outside of private network, but SSL isn't enabled (`rfs ... -o ssl`)");
		WARN("%s\n", "WARNING: BTW, check out SECURITY NOTES section in rfs man page (`man rfs`) for default security policy of remotefs");
	}
}

static int check_client_ssl(const struct rfs_instance *instance)
{
	return check_ssl(choose_ssl_client_method(), 
		instance->config.ssl_key_file, 
		instance->config.ssl_cert_file, 
		instance->config.ssl_ciphers);
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
		check_protected_connection(instance->config.host);
	}
	else
	{
		check_client_ssl(instance);
	}
#endif
	if (instance->config.auth_user != NULL 
	&& instance->config.auth_passwd_file != NULL)
	{
		check_passwd_file(instance->config.auth_passwd_file);
	}
}

