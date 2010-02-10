/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#if defined QNX
#include <sys/select.h>
#endif

#include "buffer.h"
#include "command.h"
#include "config.h"
#include "exports.h"
#include "id_lookup.h"
#include "instance_server.h"
#include "keep_alive_server.h"
#include "list.h"
#include "passwd.h"
#include "resume/cleanup.h"
#include "scheduling.h"
#include "sendrecv_server.h"
#include "server.h"
#include "server_handlers_sync.h"
#include "sockets.h"

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

	server_keep_alive_update(instance);

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

static int handle_connection(struct rfsd_instance *instance, int client_socket, const struct sockaddr_storage *client_addr)
{
	instance->sendrecv.socket = client_socket;
	instance->sendrecv.connection_lost = 0;

	srand(time(NULL));
	
	server_keep_alive_update(instance);
	alarm(server_keep_alive_period());
	
	struct command current_command = { 0 };
	
	while (1)
	{	
		rfs_receive_cmd(&instance->sendrecv, &current_command);
		
		if (instance->sendrecv.connection_lost != 0)
		{
			server_close_connection(instance);
			return 1;
		}
		
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
	
	cleanup_files(&instance->cleanup.open_files);

#ifdef WITH_MEMCACHE
	destroy_memcache(&instance->memcache);
#endif
	
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
		free(instance->server.mounted_export);
	}
	
	destroy_uids_lookup(&instance->id_lookup.uids);
	destroy_gids_lookup(&instance->id_lookup.gids);
	
#ifdef RFS_DEBUG
	dump_sendrecv_stats(&instance->sendrecv);
#endif
}

static int create_pidfile(const char *pidfile)
{
	FILE *fp = fopen(pidfile, "wt");
	if (fp == NULL)
	{
		return -1;
	}
	
	if (fprintf(fp, "%lu", (long unsigned)getpid()) < 1)
	{
		fclose(fp);
		return -1;
	}
	
	fclose(fp);
	
	return 0;
}

static int start_listening(struct list *addresses, int port, unsigned force_ipv4, unsigned force_ipv6, int *listen_sockets, unsigned *listen_number)
{
	int max_socket_number = -1;
	unsigned max_listen_number = *listen_number;
	*listen_number = 0;

	const struct list *address_item = addresses;
	while (address_item != NULL)
	{
		if (*listen_number + 1 > max_listen_number)
		{
			ERROR("Too many addresses to listen: max is %d\n", max_listen_number);
			return -1;
		}
		
		const char *address = (const char *)address_item->data;
		int listen_family = -1;

		struct sockaddr_in addr = { 0 };
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);

#ifdef WITH_IPV6
		struct sockaddr_in6 addr6 = { 0 };
		addr6.sin6_family = AF_INET6;
		addr6.sin6_port = htons(port);
#endif
		
		errno = 0;
		if (inet_pton(AF_INET, address, &(addr.sin_addr)) == 1)
		{
			listen_family = AF_INET;
		}
#ifdef WITH_IPV6
		else if (inet_pton(AF_INET6, address, &(addr6.sin6_addr)) == 1)
		{
			listen_family = AF_INET6;
		}
#endif
		
		if (listen_family == -1)
		{
			ERROR("Invalid address for listening to: %s\n", address);
			return -1;
		}

		int listen_socket = -1;

		errno = 0;
		if (listen_family == AF_INET)
		{
			listen_socket = socket(PF_INET, SOCK_STREAM, 0);
		}
#ifdef WITH_IPV6
		else if (listen_family == AF_INET6)
		{
			listen_socket = socket(PF_INET6, SOCK_STREAM, 0);
		}
#endif

		if (listen_socket == -1)
		{	
			ERROR("Error creating listen socket for %s: %s\n", address, strerror(errno));
			return 1;
		}

		listen_sockets[*listen_number] = listen_socket;
		++(*listen_number);
		max_socket_number = (max_socket_number < listen_socket ? listen_socket : max_socket_number);

		errno = 0;
		if (setup_socket_reuse(listen_socket, 1) != 0)
		{
			ERROR("Error setting reuse option for %s: %s\n", address, strerror(errno));
			return -1;
		}

#if defined WITH_IPV6
		if (force_ipv6 != 0)
		{
			if (setup_socket_ipv6_only(listen_socket) != 0)
			{
				ERROR("Error setting IPv6-only option for %s: %s\n", address, strerror(errno));
				return -1;
			}
		}
#endif

		if (listen_family == AF_INET)
		{
			errno = 0;
			if (bind(listen_socket, (struct sockaddr*)&addr, sizeof(addr)) != 0)
			{
				ERROR("Error binding to %s: %s\n", address, strerror(errno));
				return -1;
			}

		}
#ifdef WITH_IPV6
		else if (listen_family == AF_INET6)
		{			
			errno = 0;
			if (bind(listen_socket, (struct sockaddr*)&addr6, sizeof(addr6)) != 0)
			{
				ERROR("Error binding to %s: %s\n", address, strerror(errno));
				return -1;
			}
		}
#endif

		if (listen(listen_socket, LISTEN_BACKLOG) != 0)
		{
			ERROR("Error listening to %s: %s\n", address, strerror(errno));
			return -1;
		}

#ifdef WITH_IPV6
		DEBUG("listening to %s interface: %s (%d)\n", listen_family == AF_INET ? "IPv4" : "IPv6", address, port);
#else
		DEBUG("listening to IPv4 interface: %s (%d)\n", address, port);
#endif
			
		address_item = address_item->next;
	}

	return max_listen_number;
}

static int start_accepting(struct rfsd_instance *instance, int *listen_sockets, unsigned listen_number, int max_socket_number)
{
	while (1)
	{
		fd_set listen_set;

		FD_ZERO(&listen_set);
		unsigned i = 0; for (; i < listen_number; ++i)
		{
			FD_SET(listen_sockets[i], &listen_set);
		}
		
		errno = 0;
		int retval = select(max_socket_number + 1, &listen_set, NULL, NULL, NULL);

		if (errno == EINTR || errno == EAGAIN)
		{
			continue;
		}

		if (retval < 0)
		{
			DEBUG("select retval: %d: %s\n", retval, strerror(errno));
			return -1;
		}
			
		DEBUG("select retval: %d\n", retval);

		if (retval < 1)
		{
			continue;
		}

		for (i = 0; i < listen_number; ++i)
		{
			if (FD_ISSET(listen_sockets[i], &listen_set))
			{
				int listen_socket = listen_sockets[i];
				
				struct sockaddr_storage client_addr;
				socklen_t addr_len = sizeof(client_addr);

				int client_socket = accept(listen_socket, (struct sockaddr *)&client_addr, &addr_len);
				if (client_socket == -1)
				{
					continue;
				}
		
#ifdef RFS_DEBUG
				char straddr[256] = { 0 };
#ifdef WITH_IPV6
				if (client_addr.ss_family == AF_INET6)
				{
					inet_ntop(AF_INET6, &((struct sockaddr_in6*)&client_addr)->sin6_addr, straddr, sizeof(straddr));
				}
				else
#endif
				{
					inet_ntop(AF_INET, &((struct sockaddr_in*)&client_addr)->sin_addr, straddr, sizeof(straddr));
				}

				DEBUG("incoming connection from %s\n", straddr);
#endif /* RFS_DEBUG */
				
				if (fork() == 0) /* child */
				{
					unsigned j = 0; for (; j < listen_number; ++j)
					{
						close(listen_sockets[j]);
					}

#ifdef SCHEDULING_AVAILABLE
					setup_socket_ndelay(client_socket, 1);
					set_scheduler();
#endif

					return handle_connection(instance, client_socket, &client_addr);
				}
				else
				{
					close(client_socket);
				}		
			}
		}
	}

}

int start_server(struct rfsd_instance *instance, unsigned daemonize, unsigned force_ipv4, unsigned force_ipv6)
{
	if (create_pidfile(instance->config.pid_file) != 0)
	{
		ERROR("Error creating pidfile: %s\n", instance->config.pid_file);
		release_server(instance);
		return 1;
	}

	int listen_sockets[MAX_LISTEN_ADDRESSES];
	unsigned listen_number = sizeof(listen_sockets) / sizeof(listen_sockets[0]);
	int i = 0; for (; i < listen_number; ++i)
	{
		listen_sockets[i] = -1;
	}

	int max_socket_number = start_listening(
		instance->config.listen_addresses, 
		instance->config.listen_port, 
		force_ipv4, 
		force_ipv6, 
		listen_sockets, 
		&listen_number);
	
	if (max_socket_number < 0)
	{
		return -1;
	}
	
	/* the server should not apply it own mask while mknod
	file or directory creation is called. These settings
	are allready done by the client */
	umask(0);

#ifndef RFS_DEBUG
	if (chdir("/") != 0)
	{
		return 1;
	}
	
	if (daemonize)
	{
		fclose(stdin);
		fclose(stdout);
		fclose(stderr);
	}
#endif
	
	return start_accepting(instance, listen_sockets, listen_number, max_socket_number);
}

void release_server(struct rfsd_instance *instance)
{
	release_exports(&(instance->exports.list));
	release_passwords(&(instance->passwd.auths));

	unlink(instance->config.pid_file);
	release_rfsd_instance(instance);
}

void stop_server(struct rfsd_instance *instance)
{
	server_close_connection(instance);
	release_server(instance);

	exit(0);
}

void check_keep_alive(struct rfsd_instance *instance)
{
	if (server_keep_alive_locked(instance) != 0
	&& server_keep_alive_expired(instance) == 0)
	{
		DEBUG("%s\n", "keep alive expired");
		server_close_connection(instance);
		exit(1);
	}
	
	alarm(server_keep_alive_period());
}

