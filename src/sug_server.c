/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"
#include "instance_server.h"
#include "list.h"
#include "passwd.h"
#include "ssl/server.h"
#include "sug_common.h"
#include "utils.h"

static int check_listen_addresses(const struct list *addresses)
{
	/* resolving won't happen for listen address,
	so this is always an ip-address */

	const struct list *address_item = addresses;
	while (address_item != NULL)
	{
		const char *address = (const char *)(address_item->data);
	
		if (
#ifdef WITH_IPV6
		is_ipv6_local(address) == 0 &&
#endif
		is_ipv4_local(address) == 0)
		{
			WARN("WARNING: You are listening on interface outside of private network (%s).\n", address);
			return -1;
		}

		address_item = address_item->next;
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
	
	return ret;
}

static int check_root_uid()
{
	if (getuid() != 0) /* i'm sorry Dave, but i can't do that */
	{
		WARN("WARNING: You can't run rfsd without root privileges: they are required to chroot to export directory. " \
		"However, this warning can be disabled with -q option (`rfsd -q`) if you believe your OS will let you chroot without root privileges, "
		"but be prepared to get \"%s\" on client side.\n", 
		strerror(EPERM));

		return -1;
	}

	return 0;
}

#ifdef WITH_SSL
int check_server_ssl(const struct rfsd_instance *instance)
{
	return check_ssl(choose_ssl_server_method(), 
		instance->config.ssl_key_file, 
		instance->config.ssl_cert_file, 
		instance->config.ssl_ciphers);
}
#endif

int suggest_server(const struct rfsd_instance *instance)
{
	int ret = 0;
	
	if (check_listen_addresses(instance->config.listen_addresses) != 0)
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

	if (check_root_uid() != 0)
	{
		ret = -1;
	}

#ifdef WITH_SSL
	if (check_server_ssl(instance) != 0)
	{
		ret = -1;
	}
#endif
	
	if (ret != 0)
	{
		INFO("%s\n", "INFO: Please consider SECURITY NOTES section of rfsd man page (`man rfsd`)");
	}
	
	return ret;
}
