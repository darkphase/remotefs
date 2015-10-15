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
#include "handling.h"
#include "id_lookup.h"
#include "instance_server.h"
#include "keep_alive_server.h"
#include "list.h"
#include "passwd.h"
#include "resume/cleanup.h"
#include "scheduling.h"
#include "sendrecv_server.h"
#include "server.h"
#include "sockets.h"

static int handle_connection(struct rfsd_instance *instance, int client_socket, const struct sockaddr_storage *client_addr)
{
	instance->sendrecv.socket = client_socket;
	instance->sendrecv.connection_lost = 0;

	srand(time(NULL));

	server_keep_alive_update(instance);
	alarm(server_keep_alive_period());

	struct rfs_command current_command = { 0 };

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

static int start_listening(struct rfs_list *addresses, int port, int *listen_sockets, unsigned *listen_number)
{
	int max_socket_number = -1;
	unsigned max_listen_number = *listen_number;
	*listen_number = 0;

	const struct rfs_list *address_item = addresses;
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
		if (listen_family == AF_INET6)
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

int start_server(struct rfsd_instance *instance, unsigned daemonize)
{
	if (create_pidfile(instance->config.pid_file) != 0)
	{
		ERROR("Error creating pidfile: %s\n", instance->config.pid_file);
		release_server(instance);
		return 1;
	}

	int listen_sockets[MAX_LISTEN_ADDRESSES];
	unsigned listen_number = sizeof(listen_sockets) / sizeof(listen_sockets[0]);

	unsigned i = 0; for (; i < listen_number; ++i)
	{
		listen_sockets[i] = -1;
	}

	int max_socket_number = start_listening(
		instance->config.listen_addresses,
		instance->config.listen_port,
		listen_sockets,
		&listen_number);

	if (max_socket_number < 0)
	{
		return -1;
	}

	DEBUG("send timeout: %ld (usecs)\n", instance->sendrecv.send_timeout);
	DEBUG("recv timeout: %ld (usecs)\n", instance->sendrecv.recv_timeout);

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
