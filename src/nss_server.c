/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifdef WITH_UGO

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>

#include "buffer.h"
#include "command.h"
#include "config.h"
#include "instance_client.h"
#include "list.h"
#include "nss_server.h"
#include "sendrecv.h"

static char* nss_socket_name(struct rfs_instance *instance)
{
	char *name = get_buffer(FILENAME_MAX + 1);

	if (name == NULL)
	{
		return NULL;
	}
	
	uid_t uid = instance->client.my_uid;
	pid_t pid = instance->client.my_pid;

	if (snprintf(name, 
		FILENAME_MAX, 
		"%s%d-%s.%d", 
		NSS_SOCKETS_DIR, 
		uid, 
		instance->config.host, 
		pid) >= FILENAME_MAX)
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

static int nss_answer(int sock, struct answer *ans, const char *data)
{
#ifdef RFS_DEBUG
	dump_answer(ans);
#endif

	if (send(sock, ans, sizeof(*ans), 0) != sizeof(*ans))
	{
		return -errno;
	}

	if (ans->data_len != 0 
	&& data != NULL)
	{
		if (send(sock, data, ans->data_len, 0) != ans->data_len)
		{
			return -errno;
		}
	}

	return 0;
}

static unsigned check_name(int sock, const struct list *names, struct command *cmd)
{
	char *name = get_buffer(cmd->data_len);

	if (recv(sock, name, cmd->data_len, 0) != cmd->data_len)
	{
		return errno;
	}

	if (strlen(name) != cmd->data_len - 1)
	{
		struct answer ans = { cmd->command, 0, -1, EINVAL };
		nss_answer(sock, &ans, NULL);
		return -EINVAL;
	}
	
	const struct list *entry = names;
	unsigned name_valid = 0;
	while (entry != NULL)
	{
		const char *entry_name = (const char *)entry->data;
	
		if (strcmp(entry_name, name) == 0)
		{
			name_valid = 1;
			break;
		}
		
		entry = entry->next;
	}
	
	DEBUG("name (%s) valid: %d\n", name, name_valid);
		
	free_buffer(name);
	
	struct answer ans = { cmd->command, 0, 0, (name_valid != 0 ? 0 : EINVAL) };
	int answer_ret = nss_answer(sock, &ans, NULL);

	return answer_ret;
}

static int process_checkuser(struct rfs_instance *instance, int sock, struct command *cmd)
{	
	DEBUG("%s\n", "processing checkuser");
	return check_name(sock, instance->nss.users_storage, cmd);
}

static int process_checkgroup(struct rfs_instance *instance, int sock, struct command *cmd)
{
	DEBUG("%s\n", "processing checkgroup");
	return check_name(sock, instance->nss.groups_storage, cmd);
}

static int send_names(int sock, const struct list *names, enum server_commands cmd_id)
{
	const struct list *name_entry = names;

	while (name_entry != NULL)
	{
		struct answer ans = { cmd_id, strlen(name_entry->data) + 1, 0, 0 };

		int answer_ret = nss_answer(sock, &ans, name_entry->data);
		if (answer_ret != 0)
		{
			return answer_ret;
		}
		
		name_entry = name_entry->next;
	}
	
	struct answer ans = { cmd_id, 0, 0, 0 };
	int answer_ret = nss_answer(sock, &ans, NULL);

	return answer_ret;
}

static int process_getusers(struct rfs_instance *instance, int sock, struct command *cmd)
{
	return send_names(sock, instance->nss.users_storage, cmd->command);
}

static int process_getgroups(struct rfs_instance *instance, int sock, struct command *cmd)
{
	return send_names(sock, instance->nss.groups_storage, cmd->command);
}

static int process_command(struct rfs_instance *instance, int sock, struct command *cmd)
{
	switch (cmd->command)
	{
	case cmd_checkuser: 
		return process_checkuser(instance, sock, cmd);
	
	case cmd_checkgroup: 
		return process_checkgroup(instance, sock, cmd);

	case cmd_getusers:
		return process_getusers(instance, sock, cmd);
	
	case cmd_getgroups:
		return process_getgroups(instance, sock, cmd);

	default: 			
		{
		struct answer ans = { cmd->command, 0, -1, ENOTSUP };
		return nss_answer(sock, &ans, NULL);
		}
	}
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

#ifdef RFS_DEBUG
		dump_command(&cmd);
#endif

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
		"%s %s %s", 
		RFS_NSS_BIN, 
		RFS_NSS_START_OPTION, 
		instance->config.host);

	DEBUG("trying to start rfs_nss: %s\n", cmd_line);

	int rfs_nss_ret = system(cmd_line);

	DEBUG("rfs_nss return: %d\n", rfs_nss_ret);

	if (rfs_nss_ret != 0)
	{
		instance->nss.use_nss = 0;
	}

	return 0;
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
			"%s %s %s", 
			RFS_NSS_BIN, 
			RFS_NSS_STOP_OPTION, 
			instance->config.host);

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

#else
int nss_server_c_empty_module_makes_suncc_upset = 0;
#endif /* WITH_UGO */

