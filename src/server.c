/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#if defined FREEBSD || defined QNX
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#endif

#include "buffer.h"
#include "command.h"
#include "config.h"
#include "cleanup.h"
#include "exports.h"
#include "id_lookup.h"
#include "instance_server.h"
#include "keep_alive_server.h"
#include "passwd.h"
#include "scheduling.h"
#include "sendrecv.h"
#include "server.h"
#include "server_handlers_sync.h"

static int _reject_request(struct rfsd_instance *instance, const struct command *cmd, int32_t ret_errno, unsigned data_is_in_queue)
{
	struct answer ans = { cmd->command, 0, -1, ret_errno };
	
	if (data_is_in_queue != 0 
	&& rfs_ignore_incoming_data(&instance->sendrecv, cmd->data_len) != cmd->data_len)
	{
		return -1;
	}
	
	return rfs_send_answer(&instance->sendrecv, &ans) != -1 ? 0 : -1;
}

int reject_request(struct rfsd_instance *instance, const struct command *cmd, int32_t ret_errno)
{
	return _reject_request(instance, cmd, ret_errno, 0);
}

int reject_request_with_cleanup(struct rfsd_instance *instance, const struct command *cmd, int32_t ret_errno)
{
	return _reject_request(instance, cmd, ret_errno, 1);
}

int handle_command(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	if (cmd->command <= cmd_first
	|| cmd->command >= cmd_last)
	{
		reject_request_with_cleanup(instance, cmd, EINVAL);
		return -1;
	}

	update_keep_alive(instance);

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
	
	case cmd_setsocktimeout:
		return handle_setsocktimeout(instance, client_addr, cmd);
	
	case cmd_setsockbuffer:
		return handle_setsockbuffer(instance, client_addr, cmd);
	
	case cmd_enablessl:
#ifdef WITH_SSL
		return handle_enablessl(instance, client_addr, cmd);
#else
		return reject_request_with_cleanup(instance, cmd, ENOTSUP) == 0 ? 1 : -1;
#endif
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
#if (defined SOLARIS || defined QNX) && defined WITH_PAUSE
		pause_rdwr();
#endif
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

	case cmd_truncate:
		return handle_truncate(instance, client_addr, cmd);

	case cmd_write:
#if (defined SOLARIS || defined QNX) && defined WITH_PAUSE
		pause_rdwr();
#endif
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
		return reject_request_with_cleanup(instance, cmd, EACCES) == 0 ? 1 : -1;
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
#if defined WITH_ACL
	case cmd_getxattr:
		return handle_getxattr(instance, client_addr, cmd);
	case cmd_setxattr:
		return handle_setxattr(instance, client_addr, cmd);
#else
	case cmd_getxattr:
	case cmd_setxattr:
		return reject_request_with_cleanup(instance, cmd, ENOTSUP) == 0 ? 1 : -1;
#endif
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

#ifndef WITH_IPV6
int handle_connection(struct rfsd_instance *instance, int client_socket, const struct sockaddr_in *client_addr)
#else
int handle_connection(struct rfsd_instance *instance, int client_socket, const struct sockaddr_storage *client_addr)
#endif
{
	instance->sendrecv.socket = client_socket;
	instance->sendrecv.connection_lost = 0;

	srand(time(NULL));
	
	update_keep_alive(instance);
	alarm(keep_alive_period());
	
	struct command current_command = { 0 };
	
	while (1)
	{	
		rfs_receive_cmd(&instance->sendrecv, &current_command);
		
		if (instance->sendrecv.connection_lost != 0)
		{
			if (((struct sockaddr_in*)&client_addr)->sin_family == AF_INET)
			{
				char straddr[INET_ADDRSTRLEN + 1] = { 0 };
				struct sockaddr_in *sa = (struct sockaddr_in*) client_addr;
				
				inet_ntop(AF_INET, &sa->sin_addr, straddr, sizeof(straddr));
				DEBUG("connection to %s is lost\n",straddr);
			}
#ifdef WITH_IPV6
			else
			{
				char straddr[INET6_ADDRSTRLEN + 1] = { 0 };
				struct sockaddr_in6 *sa = (struct sockaddr_in6*)client_addr;
				
				inet_ntop(AF_INET6, &sa->sin6_addr, straddr, sizeof(straddr));
				DEBUG("connection to %s is lost\n", straddr);
			}
#endif
			server_close_connection(instance);
			
			return 1;
		}
		
#ifdef RFS_DEBUG
		dump_command(&current_command);
#endif
		if (handle_command(instance, (struct sockaddr_in*)client_addr, &current_command) == -1)
		{
			DEBUG("command executed with internal error: %s\n", describe_command(current_command.command));
			server_close_connection(instance);
			
			return 1;
		}
	}
}

void server_close_connection(struct rfsd_instance *instance)
{
	if (instance->sendrecv.socket != -1)
	{
		shutdown(instance->sendrecv.socket, SHUT_RDWR);
		close(instance->sendrecv.socket);
	}
	
	cleanup_files(instance);
	
	if (instance->server.auth_user != NULL)
	{
		free(instance->server.auth_user);
	}
	
	if (instance->server.auth_passwd != NULL)
	{
		free(instance->server.auth_passwd);
	}
	
	release_passwords(&instance->passwd.auths);
	release_exports(&instance->exports.list);
	
	if (instance->server.mounted_export != NULL)
	{
		free_buffer(instance->server.mounted_export);
	}
	
	destroy_uids_lookup(&instance->id_lookup.uids);
	destroy_gids_lookup(&instance->id_lookup.gids);
	
#ifdef RFS_DEBUG
	dump_sendrecv_stats(&instance->sendrecv);
#endif
}

