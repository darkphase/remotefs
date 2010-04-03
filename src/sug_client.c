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
#include "sug_client.h"
#include "utils.h"

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
	if (instance->config.auth_user != NULL 
	&& instance->config.auth_passwd_file != NULL)
	{
		check_passwd_file(instance->config.auth_passwd_file);
	}
}
