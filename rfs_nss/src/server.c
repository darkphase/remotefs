/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>

#include "common.h"
#include "config.h"
#include "global_lock.h"
#include "maintenance.h"
#include "manage_server.h"
#include "nss_cmd.h"
#include "processing.h"
#include "processing_common.h"
#include "processing_ent.h"
#include "server.h"

static struct config *server_config = NULL;

static void signal_handler(int sig, siginfo_t *info, void *null)
{
	switch (sig)
	{
		case SIGINT:
		case SIGTERM:
		case SIGHUP:
			stop_server(server_config, 0);
			break;
		
		case SIGPIPE:
			break;

		case SIGCHLD:
			{
			int status = -1;
			waitpid(info->si_pid, &status, 1);

			DEBUG("child process exited with ret code %d\n", status);
			}
			break;
	}
}

static void install_signal_handlers()
{
	struct sigaction action;
	memset(&action, 0, sizeof(action));

	action.sa_sigaction = signal_handler;
	action.sa_flags = SA_SIGINFO | SA_RESTART;

	sigaction(SIGTERM, &action, NULL);
	sigaction(SIGINT, &action, NULL);
	sigaction(SIGPIPE, &action, NULL);
	sigaction(SIGHUP, &action, NULL);
	sigaction(SIGCHLD, &action, NULL);
}

static int process_command(int sock, const struct nss_command *cmd, struct config *config)
{
	DEBUG("processing command: %s\n", describe_nss_command(cmd->command));

	switch (cmd->command)
	{
		case cmd_inc:      return process_inc(sock, cmd, config);
		case cmd_dec:      return process_dec(sock, cmd, config);
		case cmd_addserver:return process_addserver(sock, cmd, config);
		case cmd_adduser:  return process_adduser(sock, cmd, config);
		case cmd_addgroup: return process_addgroup(sock, cmd, config);

		case cmd_getpwnam: return process_getpwnam(sock, cmd, config);
		case cmd_getpwuid: return process_getpwuid(sock, cmd, config);
		case cmd_getgrnam: return process_getgrnam(sock, cmd, config);
		case cmd_getgrgid: return process_getgrgid(sock, cmd, config);

		case cmd_getpwent: return process_getpwent(sock, cmd, config);
		case cmd_setpwent: return process_setpwent(sock, cmd, config);
		case cmd_endpwent: return process_endpwent(sock, cmd, config);
		case cmd_getgrent: return process_getgrent(sock, cmd, config);
		case cmd_setgrent: return process_setgrent(sock, cmd, config);
		case cmd_endgrent: return process_endgrent(sock, cmd, config);

		default:
			reject_request(sock, cmd, EINVAL);
			return -1;
	}
}

int start_listen(uid_t uid, struct config *config)
{
	DEBUG("starting server for uid %d\n", uid);

	char *socketname = socket_name(config->allow_other ? (uid_t)-1 : uid);

	DEBUG("creating socket for %s\n", socketname);

	int sock = socket(PF_UNIX, SOCK_STREAM, 0);
	if (sock == -1)
	{
		return -errno;
	}
	
	struct sockaddr_un address = { 0 };
	strcpy(address.sun_path, socketname);
	address.sun_family = AF_UNIX;

	if (bind(sock, (struct sockaddr *)&address, sizeof(struct sockaddr_un)) == -1)
	{
		int saved_errno = errno;
		close(sock);
		unlink(socketname);
		return -saved_errno;
	}

	chmod(socketname, (config->allow_other ? 0666 : 0600));

	if (listen(sock, 1) != 0)
	{
		int saved_errno = errno;
		close(sock);
		unlink(socketname);
		return -saved_errno;
	}

	config->sock = sock;
	config->socketname = socketname;
	
	server_config = config;
	install_signal_handlers();

	start_maintenance_thread(config);

	return 0;
}

int start_accepting(struct config *config)
{
	int sock = config->sock;

	while (sock != -1)
	{
		struct sockaddr_un client_addr = { 0 };
		socklen_t client_addr_size = sizeof(client_addr);
		
		DEBUG("waiting for connection on sock %d\n", sock);

		int client_sock = accept(sock, (struct sockaddr *)&client_addr, &client_addr_size);
		
		if (client_sock <= 0)
		{
			break;
		}

		DEBUG("%s\n", "reading command");
	
		struct nss_command cmd = { 0 };

		int proc_ret = -1;

		if (recv(client_sock, &cmd, sizeof(cmd), 0) <= 0)
		{
			goto skip;
		}

		proc_ret = process_command(client_sock, &cmd, config);

skip:

		if (cmd.command != cmd_addserver)
		{
			DEBUG("closing socket %d\n", client_sock);
			shutdown(client_sock, SHUT_RDWR);
		}
			
		close(client_sock);

		if (cmd.command == cmd_dec
		&& proc_ret == 0
		&& config->connections == 0)
		{
			DEBUG("%s\n", "no more connections: exiting");

			break;
		}
	}

	stop_server(config, 0);

	return 0;
}

int do_stop_server(struct config *config, int ret_code)
{
	DEBUG("closing socket %d for %s\n", config->sock, config->socketname);

	shutdown(config->sock, SHUT_RDWR);
	close(config->sock);

	config->sock = -1;

	unlink(config->socketname);
	free(config->socketname);

	config->socketname = NULL;

	if (got_global_lock(config))
	{
		release_global_lock(config);
	}

	exit(ret_code);
}

int stop_server(struct config *config, int ret_code)
{
	stop_maintenance_thread(config);

	return do_stop_server(config, ret_code);
}

