/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include "../options.h"

#if defined RFSNSS_AVAILABLE

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>

#include "../buffer.h"
#include "../command.h"
#include "../config.h"
#include "../instance_client.h"
#include "../list.h"
#include "operations_nss.h"
#include "processing.h"
#include "server.h"

static char* nss_socket_name(struct rfs_instance *instance)
{
	char *name = malloc(FILENAME_MAX + 1);

	if (name == NULL)
	{
		return NULL;
	}
	
	uid_t uid = instance->client.my_uid;
	pid_t pid = instance->client.my_pid;

	if (snprintf(name, 
		FILENAME_MAX, 
		"%srfs_nss-%d-%s.%lu", 
		NSS_SOCKETS_DIR, 
		uid, 
		instance->config.host, 
		(long unsigned)pid) >= FILENAME_MAX)
	{
		return NULL;
	}

	return name;
}

int nss_create_socket(struct rfs_instance *instance)
{
	char *socket_name = nss_socket_name(instance);

	if (socket_name == NULL)
	{
		return -ECANCELED;
	}

	DEBUG("socket name: %s\n", socket_name);

	int sock = socket(PF_UNIX, SOCK_STREAM, 0);
	if (sock == -1)
	{
		return -errno;
	}

	struct sockaddr_un address = { 0 };
	strcpy(address.sun_path, socket_name);
	address.sun_family = AF_UNIX;

	if (bind(sock, (struct sockaddr *)&address, sizeof(struct sockaddr_un)) == -1)
	{
		int saved_errno = errno;
		close(sock);
		unlink(socket_name);
		return -saved_errno;
	}

	chmod(socket_name, 0600);

	free(socket_name);

	instance->nss.socket = sock;

	return 0;
}

int nss_close_socket(struct rfs_instance *instance)
{
	if (instance->nss.socket != -1)
	{
		shutdown(instance->nss.socket, SHUT_RDWR);
		close(instance->nss.socket);

		instance->nss.socket = -1;

		char *socket_name = nss_socket_name(instance);
		unlink(socket_name);
		free(socket_name);
	}

	return 0;
}

static void* nss_server_proc(void *void_instance)
{
	struct rfs_instance *instance = (struct rfs_instance *)void_instance;

	if (instance->nss.socket == -1)
	{
		pthread_exit(NULL);
	}

	int sock = instance->nss.socket;

	if (listen(sock, 1) != 0)
	{
		pthread_exit(NULL);
	}
	
	DEBUG("%s\n", "NSS started");

	rfs_sem_post(&instance->nss.thread_ready);

	while (instance->nss.socket != -1)
	{
		struct sockaddr_un client_addr = { 0 };
		socklen_t client_addr_size = sizeof(client_addr);
		
		DEBUG("%s\n", "waiting for connection");

		int client_sock = accept(sock, (struct sockaddr *)&client_addr, &client_addr_size);
		
		DEBUG("client socket: %d\n", client_sock);

		if (client_sock <= 0)
		{
			break;
		}

		DEBUG("%s\n", "receiving command");

		struct command cmd = { 0 };
		int done = recv(client_sock, &cmd, sizeof(cmd), 0);
		
		if (done <= 0)
		{
			break;
		}

#ifdef RFS_DEBUG
		dump_command(&cmd);
#endif

		process_command(instance, client_sock, &cmd);

		DEBUG("%s\n", "closing socket");

		shutdown(client_sock, SHUT_RDWR);
		close(client_sock);
	}

	pthread_exit(NULL);
}

int start_nss_server(struct rfs_instance *instance)
{
	DEBUG("%s\n", "starting NSS server");

	int create_sock_ret = nss_create_socket(instance);
	if (create_sock_ret != 0)
	{
		return create_sock_ret;
	}
	
	if (pthread_create(&instance->nss.server_thread, NULL, nss_server_proc, (void *)instance) != 0)
	{
		instance->nss.server_thread = 0;
		nss_close_socket(instance);
		return -1;
	}

	DEBUG("%s\n", "waiting for NSS thread to become ready");
	if (rfs_sem_wait(&instance->nss.thread_ready) != 0)
	{
		return -1;
	}

	char cmd_line[4096] = { 0 };

	snprintf(cmd_line, 
		sizeof(cmd_line) - 1, 
		"%s %s %s %s %s >/dev/null 2>&1", 
		RFS_NSS_BIN, 
		RFS_NSS_RFSHOST_OPTION, 
		instance->config.host, 
		instance->config.allow_other ? RFS_NSS_SHARED_OPTION : "", 
		RFS_NSS_START_OPTION);

	DEBUG("trying to start rfs_nss: %s\n", cmd_line);

	int rfs_nss_ret = system(cmd_line);

	DEBUG("rfs_nss return: %d\n", rfs_nss_ret);

	return (rfs_nss_ret == 0 ? 0 : -1);
}

int stop_nss_server(struct rfs_instance *instance)
{
	DEBUG("%s\n", "stopping NSS server");

	nss_close_socket(instance);

	if (instance->nss.server_thread != 0)
	{
		pthread_join(instance->nss.server_thread, NULL);
		instance->nss.server_thread = 0;
	}
	
	destroy_list(&instance->nss.users_storage);
	destroy_list(&instance->nss.groups_storage);

	if (instance->nss.use_nss != 0)
	{
		char cmd_line[4096] = { 0 };

		snprintf(cmd_line, 
			sizeof(cmd_line) - 1, 
			"%s %s %s %s %s >/dev/null 2>&1", 
			RFS_NSS_BIN, 
			RFS_NSS_RFSHOST_OPTION, 
			instance->config.host, 
			instance->config.allow_other ? RFS_NSS_SHARED_OPTION : "", 
			RFS_NSS_STOP_OPTION);

		DEBUG("trying to stop rfs_nss: %s\n", cmd_line);

		int rfs_nss_ret = system(cmd_line);
	
		DEBUG("rfs_nss return: %d\n", rfs_nss_ret);

		if (rfs_nss_ret != 0)
		{
			return -1;
		}
	}

	return 0;
}

unsigned is_nss_running(struct rfs_instance *instance)
{
	return (instance->nss.server_thread != 0);
}

int init_nss_server(struct rfs_instance *instance, unsigned show_errors)
{
	if ((instance->client.export_opts & OPT_UGO) != 0)
	{
		if (is_nss_running(instance) == 0)
		{
			int getnames_ret = rfs_getnames(instance);
			if (getnames_ret != 0)
			{
				if (show_errors != 0)
				{
					ERROR("Error getting NSS lists from server: %s\n", strerror(-getnames_ret));
				}
				return -1;
			}

			int nss_start_ret = start_nss_server(instance);
			DEBUG("nss start ret: %d\n", nss_start_ret);
			if (nss_start_ret != 0)
			{
				if (show_errors != 0)
				{
					WARN("Error starting NSS server: %s\n", strerror(-nss_start_ret));
				}
				return -1;
			}
		}		
	}

	return 0;
}

#else
int nss_server_c_empty_module = 0;
#endif /* RFSNSS_AVAILABLE */

