/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>

#include "config.h"
#include "command.h"
#include "buffer.h"
#include "nss_server.h"
#include "instance.h"
#include "sendrecv.h"
#include "list.h"

static char* nss_socket_name(struct rfs_instance *instance)
{
	char *name = get_buffer(FILENAME_MAX + 1);

	if (name == NULL)
	{
		return NULL;
	}
	
	uid_t uid = getuid();
	pid_t pid = getpid();

	snprintf(name, 
		FILENAME_MAX + 1, 
		"%s%d-%s.%d", 
		NSS_SOCKETS_DIR, 
		uid, 
		instance->config.host, 
		pid);

	return name;
}

int nss_create_socket(struct rfs_instance *instance)
{
	char *socket_name = nss_socket_name(instance);

	if (socket_name == NULL)
	{
		return -ECANCELED;
	}

	DEBUG("%s\n", socket_name);

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
		close(sock);
		return -errno;
	}

	chmod(socket_name, 0600);

	free_buffer(socket_name);

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
		free_buffer(socket_name);
	}

	return 0;
}

static int nss_answer(int sock, uint32_t ret_errno)
{
	return (send(sock, &ret_errno, sizeof(ret_errno), 0) == sizeof(ret_errno) ? 0 : -1);
}

static unsigned check_name(const struct list *head, const char *name)
{
	const struct list *entry = head;
	while (entry != NULL)
	{
		const char *entry_name = (const char *)entry->data;
		
		if (strcmp(entry_name, name) == 0)
		{
			return 1;
		}
		
		entry = entry->next;
	}

	return 0;
}

static int process_checkuser(struct rfs_instance *instance, int sock, struct command *cmd)
{
	DEBUG("%s\n", "processing checkname");

	char *name = get_buffer(cmd->data_len);

	if (recv(sock, name, cmd->data_len, 0) != cmd->data_len)
	{
		return errno;
	}

	if (strlen(name) != cmd->data_len - 1)
	{
		nss_answer(sock, EINVAL);
		return -EINVAL;
	}

	DEBUG("checking name: %s\n", name);

	nss_answer(sock, check_name(instance->nss.users_storage, name) != 0 ? 0 : EINVAL);

	free_buffer(name);

	return 0;
}

static int process_checkgroup(struct rfs_instance *instance, int sock, struct command *cmd)
{
	DEBUG("%s\n", "processing checkgroup");

	char *name = get_buffer(cmd->data_len);

	if (recv(sock, name, cmd->data_len, 0) != cmd->data_len)
	{
		return errno;
	}

	if (strlen(name) != cmd->data_len - 1)
	{
		nss_answer(sock, EINVAL);
		return -EINVAL;
	}

	DEBUG("checking group: %s\n", name);

	nss_answer(sock, check_name(instance->nss.groups_storage, name) != 0 ? 0 : EINVAL);

	free_buffer(name);

	return 0;
}

static int process_command(struct rfs_instance *instance, int sock, struct command *cmd)
{
	switch (cmd->command)
	{
	case cmd_checkuser: 
		return process_checkuser(instance, sock, cmd);
	
	case cmd_checkgroup: 
		return process_checkgroup(instance, sock, cmd);

	default: 			
		return nss_answer(sock, -ENOTSUP);
	}
}

static void* nss_server_proc(void *void_instance)
{
	struct rfs_instance *instance = (struct rfs_instance *)void_instance;

	if (instance->nss.socket == -1)
	{
		pthread_exit(NULL);
	}

	int sock =  instance->nss.socket;

	if (listen(sock, 1) != 0)
	{
		pthread_exit(NULL);
	}
	
	DEBUG("%s\n", "NSS started");

	while (1)
	{
		struct sockaddr_un client_addr = { 0 };
		socklen_t client_addr_size = sizeof(client_addr);
		
		DEBUG("%s\n", "waiting for connection");

		int client_sock = accept(sock, (struct sockaddr *)&client_addr, &client_addr_size);
		
		DEBUG("client socket: %d\n", client_sock);

		if (client_sock == -1)
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

		process_command(instance, client_sock, &cmd);

		DEBUG("%s\n", "closing socket");

		shutdown(client_sock, SHUT_RDWR);
		close(client_sock);
	}

	return NULL;
}

int start_nss_server(struct rfs_instance *instance)
{
	DEBUG("%s\n", "starting NSS server");

	int create_sock_ret = nss_create_socket(instance);
	if (create_sock_ret != 0)
	{
		return create_sock_ret;
	}
	
	pthread_create(&instance->nss.server_thread, NULL, nss_server_proc, (void *)instance);

	return 0;
}

int stop_nss_server(struct rfs_instance *instance)
{
	DEBUG("%s\n", "stopping NSS server");

	nss_close_socket(instance);

	if (instance->nss.server_thread != 0)
	{
		return pthread_join(instance->nss.server_thread, NULL);
	}

	destroy_list(&instance->nss.users_storage);
	destroy_list(&instance->nss.groups_storage);
	
	return 0;
}

