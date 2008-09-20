/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

/* syncronized server handlers. will lock keep alive when it's needed */

#define DECLARE_AND_DECORATE(declare_func, decorate_func)          \
int declare_func(const int client_socket,                          \
	const struct sockaddr_in *client_addr,                     \
	const struct command *cmd)                                 \
{                                                                  \
	if (keep_alive_lock() == 0)                                \
	{                                                          \
		int ret = decorate_func(client_socket, client_addr, cmd); \
		return keep_alive_unlock() == 0 ? ret : -1;        \
	}                                                          \
	return -1;                                                 \
}

DECLARE_AND_DECORATE(handle_changepath, _handle_changepath)
DECLARE_AND_DECORATE(handle_closeconnection, _handle_closeconnection)
DECLARE_AND_DECORATE(handle_request_salt, _handle_request_salt)
DECLARE_AND_DECORATE(handle_auth, _handle_auth)
DECLARE_AND_DECORATE(handle_getexportopts, _handle_getexportopts)

DECLARE_AND_DECORATE(handle_mknod, _handle_mknod)
DECLARE_AND_DECORATE(handle_chmod, _handle_chmod)
DECLARE_AND_DECORATE(handle_chown, _handle_chown)
DECLARE_AND_DECORATE(handle_release, _handle_release)
DECLARE_AND_DECORATE(handle_statfs, _handle_statfs)
DECLARE_AND_DECORATE(handle_utime, _handle_utime)
DECLARE_AND_DECORATE(handle_rename, _handle_rename)
DECLARE_AND_DECORATE(handle_rmdir, _handle_rmdir)
DECLARE_AND_DECORATE(handle_unlink, _handle_unlink)
DECLARE_AND_DECORATE(handle_mkdir, _handle_mkdir)
DECLARE_AND_DECORATE(handle_write, _handle_write)
DECLARE_AND_DECORATE(handle_read, _handle_read)
DECLARE_AND_DECORATE(handle_truncate, _handle_truncate)
DECLARE_AND_DECORATE(handle_open, _handle_open)
DECLARE_AND_DECORATE(handle_readdir, _handle_readdir)
DECLARE_AND_DECORATE(handle_getattr, _handle_getattr)

#undef DECLARE_AND_DECORATE
