/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>

#include "buffer.h"
#include "command.h"
#include "config.h"
#include "exports.h"
#include "handlers.h"
#include "instance_server.h"
#include "keep_alive_server.h"
#include "sendrecv_server.h"

static int _reject_request(struct rfsd_instance *instance, const struct rfs_command *cmd, int32_t ret_errno, unsigned data_is_in_queue)
{
	DEBUG("%s\n", "rejecting request");

	struct rfs_answer ans = { cmd->command, 0, -1, ret_errno };

	if (data_is_in_queue != 0 
	&& rfs_ignore_incoming_data(&instance->sendrecv, cmd->data_len) != cmd->data_len)
	{
		return -1;
	}
	
	return rfs_send_answer(&instance->sendrecv, &ans) != -1 ? 0 : -1;
}

int reject_request(struct rfsd_instance *instance, const struct rfs_command *cmd, int32_t ret_errno)
{
	return _reject_request(instance, cmd, ret_errno, 0);
}

int reject_request_with_cleanup(struct rfsd_instance *instance, const struct rfs_command *cmd, int32_t ret_errno)
{
	return _reject_request(instance, cmd, ret_errno, 1);
}

int handle_command(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct rfs_command *cmd)
{
	if (cmd->command <= cmd_first
	|| cmd->command >= cmd_last)
	{
		reject_request_with_cleanup(instance, cmd, EINVAL);
		return -1;
	}

	server_keep_alive_update(instance);

	switch (cmd->command)
	{
	case cmd_handshake:
		return handle_handshake(instance, client_addr, cmd);
	}

	/* client should do handshake before all other commands */
	if (instance->server.hand_shaken == 0)
	{
		reject_request_with_cleanup(instance, cmd, EACCES);
		return -1;
	}

	switch (cmd->command)
	{
	case cmd_closeconnection:
		return handle_closeconnection(instance, client_addr, cmd);

	case cmd_auth:
		return handle_auth(instance, client_addr, cmd);

	case cmd_request_salt:
		return handle_request_salt(instance, client_addr, cmd);

	case cmd_changepath:
		return handle_changepath(instance, client_addr, cmd);

	case cmd_keepalive:
		return handle_keepalive(instance, client_addr, cmd);
	
	case cmd_getexportopts:
		return handle_getexportopts(instance, client_addr, cmd);
	
	case cmd_listexports:
#ifdef WITH_EXPORTS_LIST
		return handle_listexports(instance, client_addr, cmd);
#else
		return reject_request_with_cleanup(instance, cmd, ENOTSUP) == 0 ? 1 : -1;
#endif
	}

	if (instance->server.directory_mounted == 0)
	{
		reject_request_with_cleanup(instance, cmd, EACCES);
		return -1;
	}

	switch (cmd->command)
	{
	case cmd_readdir:
		return handle_readdir(instance, client_addr, cmd);
	
	case cmd_getattr:
		return handle_getattr(instance, client_addr, cmd);

	case cmd_open:
		return handle_open(instance, client_addr, cmd);

	case cmd_release:
		return handle_release(instance, client_addr, cmd);
	
	case cmd_read:
		return handle_read(instance, client_addr, cmd);
	
	case cmd_statfs:
		return handle_statfs(instance, client_addr, cmd);
	}

	if (instance->server.mounted_export == NULL
	|| (instance->server.mounted_export->options & OPT_RO) != 0)
	{
		return reject_request_with_cleanup(instance, cmd, EACCES) == 0 ? 1 : -1;
	}

	/* operation which are require write permissions */
	switch (cmd->command)
	{
	case cmd_mknod:
		return handle_mknod(instance, client_addr, cmd);

	case cmd_create:
		return handle_create(instance, client_addr, cmd);

	case cmd_truncate:
		return handle_truncate(instance, client_addr, cmd);

	case cmd_write:
		return handle_write(instance, client_addr, cmd);

	case cmd_mkdir:
		return handle_mkdir(instance, client_addr, cmd);

	case cmd_unlink:
		return handle_unlink(instance, client_addr, cmd);

	case cmd_rmdir:
		return handle_rmdir(instance, client_addr, cmd);
	
	case cmd_rename:
		return handle_rename(instance, client_addr, cmd);

	case cmd_utime:
		return handle_utime(instance, client_addr, cmd);

	case cmd_utimens:
		return handle_utimens(instance, client_addr, cmd);
	
	case cmd_lock:
		return handle_lock(instance, client_addr, cmd);

	case cmd_link:
		return handle_link(instance, client_addr, cmd);
	
	case cmd_symlink:
		return handle_symlink(instance, client_addr, cmd);
	
	case cmd_readlink:
		return handle_readlink(instance, client_addr, cmd);
	}

#ifdef WITH_UGO
	if (instance->server.mounted_export == NULL
	|| (instance->server.mounted_export->options & OPT_UGO) == 0)
	{
		return reject_request_with_cleanup(instance, cmd, ENOTSUP) == 0 ? 1 : -1;
	}

	/* operation which are require ugo set */
	switch (cmd->command)
	{
	case cmd_chmod:
		return handle_chmod(instance, client_addr, cmd);

	case cmd_chown:
		return handle_chown(instance, client_addr, cmd);
	
	case cmd_getnames:
		return handle_getnames(instance, client_addr, cmd);
#if defined ACL_AVAILABLE
	case cmd_getxattr:
		return handle_getxattr(instance, client_addr, cmd);
	case cmd_setxattr:
		return handle_setxattr(instance, client_addr, cmd);
#else
	case cmd_getxattr:
	case cmd_setxattr:
		return reject_request_with_cleanup(instance, cmd, ENOTSUP) == 0 ? 1 : -1;
#endif /* ACL_AVAILABLE */
	}
#else /* without UGO */
	switch (cmd->command)
	{	
	case cmd_chmod:
	case cmd_chown:
	case cmd_getnames:
	case cmd_getxattr:
	case cmd_setxattr:
		return reject_request_with_cleanup(instance, cmd, ENOTSUP) == 0 ? 1 : -1;
	}
#endif
	
	reject_request_with_cleanup(instance, cmd, EINVAL);
	
	return -1;
}
