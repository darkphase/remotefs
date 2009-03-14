/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

/** syncronized server handlers. will lock keep alive when it's needed */

#include "keep_alive_server.h"
#include "server_handlers.h"

/* need to define client_socket, client_addr and cmd before using this macro */
#define DECORATE(decorate_func)                                 \
	if (keep_alive_lock(instance) == 0)                         \
	{                                                           \
		int ret = (decorate_func)(instance, client_addr, cmd);  \
		return keep_alive_unlock(instance) == 0 ? ret : -1;     \
	}                                                           \
	return -1;

int handle_changepath(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	DECORATE(_handle_changepath)
}

int handle_closeconnection(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	DECORATE(_handle_closeconnection)
}

int handle_request_salt(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	DECORATE(_handle_request_salt)
}

int handle_auth(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	DECORATE(_handle_auth)
}

int handle_getexportopts(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	DECORATE(_handle_getexportopts)
}

#ifdef WITH_SSL
int handle_enablessl(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	DECORATE(_handle_enablessl)
}
#endif

#ifdef WITH_EXPORTS_LIST
int handle_listexports(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd) 
{
	DECORATE(_handle_listexports)
}
#endif

int handle_mknod(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	DECORATE(_handle_mknod)
}

int handle_chmod(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	DECORATE(_handle_chmod)
}

int handle_chown(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	DECORATE(_handle_chown)
}

int handle_release(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	DECORATE(_handle_release)
}

int handle_statfs(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	DECORATE(_handle_statfs)
}

int handle_utime(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	DECORATE(_handle_utime)
}

int handle_rename(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd) 
{
	DECORATE(_handle_rename)
}

int handle_rmdir(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd) 
{
	DECORATE(_handle_rmdir)
}

int handle_unlink(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd) 
{
	DECORATE(_handle_unlink)
}

int handle_mkdir(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd) 
{
	DECORATE(_handle_mkdir)
}

int handle_write(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd) 
{
	DECORATE(_handle_write)
}

int handle_read(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd) 
{
	DECORATE(_handle_read)
}

int handle_truncate(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd) 
{
	DECORATE(_handle_truncate)
}

int handle_open(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd) 
{
	DECORATE(_handle_open)
}

int handle_readdir(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd) 
{
	DECORATE(_handle_readdir)
}

int handle_getattr(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd) 
{
	DECORATE(_handle_getattr)
}

int handle_lock(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd) 
{
	DECORATE(_handle_lock)
}

int handle_symlink(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd) 
{
	DECORATE(_handle_symlink)
}

int handle_readlink(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd) 
{
	DECORATE(_handle_readlink)
}

int handle_link(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd) 
{
	DECORATE(_handle_link)
}

#if defined WITH_ACL
int handle_setxattr(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	DECORATE(_handle_setxattr)
}

int handle_getxattr(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	DECORATE(_handle_getxattr)
}
#endif

int handle_getnames(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	DECORATE(_handle_getnames)
}

#undef DECORATE

