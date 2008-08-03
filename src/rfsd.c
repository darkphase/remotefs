#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <malloc.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pwd.h>
#include <time.h>

#include "config.h"
#include "command.h"
#include "signals_server.h"
#include "sendrecv.h"
#include "buffer.h"
#include "server_handlers.h"
#include "alloc.h"
#include "list.h"
#include "exports.h"
#include "passwd.h"
#include "keep_alive_server.h"

static int g_client_socket = -1;
static int g_listen_socket = -1;
unsigned char directory_mounted = 0;
extern char *auth_user;
extern char *auth_passwd;

static struct list *open_files = NULL;
struct rfs_export *mounted_export = NULL;

struct rfsd_config rfsd_config;

void init_config()
{
 	rfsd_config.listen_address = "0.0.0.0";
	rfsd_config.listen_port = DEFAULT_SERVER_PORT;
	rfsd_config.worker_uid = getuid();
#ifdef RFS_DEBUG
	rfsd_config.pid_file = "./rfsd.pid";
#else
	rfsd_config.pid_file = "/var/run/rfsd.pid";
#endif // RFS_DEBUG
}

int create_pidfile(const char *pidfile)
{
	FILE *fp = fopen(pidfile, "wt");
	if (fp == NULL)
	{
		return -1;
	}
	
	if (fprintf(fp, "%u", getpid()) < 1)
	{
		fclose(fp);
		return -1;
	}
	
	fclose(fp);
	
	return 0;
}

struct list* check_file_in_open_list(int file)
{
	struct list *item = open_files;
	while (item != NULL)
	{
		if (*((int *)(item->data)) == file)
		{
			return item;
		}
		
		item = item->next;
	}
	
	return NULL;
}

int add_file_to_open_list(int file)
{
	struct list *exist = check_file_in_open_list(file);
	if (exist != NULL)
	{
		return 0;
	}

	int *handle = get_buffer(sizeof(file));
	if (handle == NULL)
	{
		return -1;
	}
	
	*handle = file;
	
	struct list *added = add_to_list(open_files, handle);
	if (added == NULL)
	{
		free_buffer(handle);
		return -1;
	}
	
	if (open_files == NULL)
	{
		open_files = added;
	}
	
	return 0;
}

int remove_file_from_open_list(int file)
{
	struct list *exist = check_file_in_open_list(file);
	if (exist == NULL)
	{
		return -1;
	}
	
	unsigned reset_head = (exist == open_files ? 1 : 0);
	struct list *next_file = remove_from_list(exist);
	if (reset_head == 1)
	{
		open_files = next_file;
	}
	
	return 0;
}

void server_close_connection(int socket)
{
	if (socket != -1)
	{
		shutdown(socket, SHUT_RDWR);
		close(socket);
	}
	
#ifdef RFS_DEBUG
	dump_sendrecv_stats();
#endif

	if (open_files != NULL)
	{
		struct list *item = open_files;
		while (item != 0)
		{
			DEBUG("closing still open handle: %d\n", *((int *)(item->data)));
			
			close(*((int *)(item->data)));
			item = item->next;
		}
		
		destroy_list(open_files);
	}
	
	if (auth_user != 0)
	{
		free(auth_user);
	}
	
	if (auth_passwd != 0)
	{
		free(auth_passwd);
	}
	
	release_passwords();
	release_exports();
	
	if (mounted_export != NULL)
	{
		free_buffer(mounted_export);
	}

	mp_force_free();
}

int _reject_request(const int client_socket, const struct command *cmd, int32_t ret_errno, unsigned data_is_in_queue)
{
	struct answer ans = { cmd->command, 0, -1, ret_errno };
	
	if (data_is_in_queue != 0 
	&& rfs_ignore_incoming_data(client_socket, cmd->data_len) != cmd->data_len)
	{
		return -1;
	}
	
	return rfs_send_answer(client_socket, &ans) != -1 ? 0 : -1;
}

int reject_request(const int client_socket, const struct command *cmd, int32_t ret_errno)
{
	return _reject_request(client_socket, cmd, ret_errno, 0);
}

int reject_request_with_cleanup(const int client_socket, const struct command *cmd, int32_t ret_errno)
{
	return _reject_request(client_socket, cmd, ret_errno, 1);
}

int handle_command(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	if (cmd->command <= cmd_first
	|| cmd->command >= cmd_last)
	{
		reject_request_with_cleanup(client_socket, cmd, EBADE);
		return -1;
	}
	
	update_keep_alive();
	
	switch (cmd->command)
	{
	case cmd_last:
	case cmd_first:
		reject_request_with_cleanup(client_socket, cmd, EBADE);
		return -1;
	}
	
	switch (cmd->command)
	{
	case cmd_closeconnection:
		return handle_closeconnection(client_socket, client_addr, cmd);
		
	case cmd_auth:
		return handle_auth(client_socket, client_addr, cmd);
		
	case cmd_request_salt:
		return handle_request_salt(client_socket, client_addr, cmd);
	
	case cmd_changepath:
		return handle_changepath(client_socket, client_addr, cmd);
	
	case cmd_keepalive:
		return handle_keepalive(client_socket, client_addr, cmd);
	}
	
	if (directory_mounted == 0)
	{
		reject_request_with_cleanup(client_socket, cmd, EACCES);
		return -1;
	}
	
	switch (cmd->command)
	{
	case cmd_readdir:
		return handle_readdir(client_socket, client_addr, cmd);
		
	case cmd_getattr:
		return handle_getattr(client_socket, client_addr, cmd);
	
	case cmd_open:
		return handle_open(client_socket, client_addr, cmd);
	
	case cmd_release:
		return handle_release(client_socket, client_addr, cmd);
		
	case cmd_read:
		return handle_read(client_socket, client_addr, cmd);
		
	case cmd_statfs:
		return handle_statfs(client_socket, client_addr, cmd);
	}
	
	DEBUG("mounted options: %d\n", mounted_export->options);
	
	if (mounted_export == NULL
	|| (mounted_export->options & opt_ro) != 0)
	{
		return reject_request_with_cleanup(client_socket, cmd, EACCES) == 0 ? 1 : -1;
	}
	
	// operation which are require write permissions
	switch (cmd->command)
	{
	case cmd_mknod:
		return handle_mknod(client_socket, client_addr, cmd);
		
	case cmd_truncate:
		return handle_truncate(client_socket, client_addr, cmd);
		
	case cmd_write:
		return handle_write(client_socket, client_addr, cmd);
		
	case cmd_mkdir:
		return handle_mkdir(client_socket, client_addr, cmd);
		
	case cmd_unlink:
		return handle_unlink(client_socket, client_addr, cmd);
		
	case cmd_rmdir:
		return handle_rmdir(client_socket, client_addr, cmd);
		
	case cmd_rename:
		return handle_rename(client_socket, client_addr, cmd);
		
	case cmd_utime:
		return handle_utime(client_socket, client_addr, cmd);
	}
	
	return -1;
}

int handle_connection(int client_socket, const struct sockaddr_in *client_addr)
{
	g_client_socket = client_socket;
	
	srand(time(NULL));
	
	update_keep_alive();
	alarm(keep_alive_period());
	
	struct command current_command = { 0 };
	
	while (1)
	{	
		int done = rfs_receive_cmd(client_socket, &current_command);
		if (done == -1 || done == 0)
		{
			DEBUG("connection to %s is lost\n", inet_ntoa(client_addr->sin_addr));
			server_close_connection(client_socket);
				
			return 1;
		}
		
		dump_command(&current_command);
			
		if (handle_command(client_socket, client_addr, &current_command) == -1)
		{
			DEBUG("command executed with internal error: %s\n", describe_command(current_command.command));
			server_close_connection(client_socket);
			
			return 1;
		}
	}
}

int start_server(const char *address, const unsigned port)
{
	install_signal_handlers_server();

	g_listen_socket = socket(PF_INET, SOCK_STREAM, 0);
	if (g_listen_socket == -1)
	{
		ERROR("Error creating socket: %s\n", strerror(errno));
		return 1;
	}
	
	struct sockaddr_in addr = { 0 };
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(address);
	
	int reuse = 1;
	if (setsockopt(g_listen_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) != 0)
	{
		ERROR("Error setting proper option to socket: %s\n", strerror(errno));
		return 1;
	}
	
	if (bind(g_listen_socket, (struct sockaddr *)&addr, sizeof(addr)) == -1)
	{
		ERROR("Error binding: %s\n", strerror(errno));
		return 1;
	}
	
	if (listen(g_listen_socket, 10) == -1)
	{
		ERROR("Error listening: %s\n", strerror(errno));
		return 1;
	}
	
	DEBUG("listening on %s (%d)\n", inet_ntoa(addr.sin_addr), port);
	
#ifndef RFS_DEBUG
	chdir("/");
	
	fclose(stdin);
	fclose(stdout);
	fclose(stderr);
#endif
	
	while (1)
	{
		struct sockaddr_in client_addr;
		socklen_t addr_len = sizeof(client_addr);
		int client_socket = accept(g_listen_socket, (struct sockaddr *)&client_addr, &addr_len);
		if (client_socket == -1)
		{
			continue;
		}
		
		DEBUG("incoming connection from: %s\n", inet_ntoa(client_addr.sin_addr));

		if (fork() == 0) // child
		{
			return handle_connection(client_socket, &client_addr);
		}
	}
	
	return 0;
}

void stop_server()
{
	server_close_connection(g_client_socket);
	if (g_listen_socket != -1)
	{
		shutdown(g_listen_socket, SHUT_RDWR);
		close(g_listen_socket);
	}
	
	unlink(rfsd_config.pid_file);
	
	exit(0);
}

void check_keep_alive()
{
	if (keep_alive_locked() != 0
	&& keep_alive_expired() == 0)
	{
		stop_server();
	}
	
	alarm(keep_alive_period());
}

void usage(const char *app_name)
{
	printf("usage: %s [options]\n"
	"\n"
	"Options:\n"
	"-h \t\t\tshow this help screen\n"
	"-a [address]\t\tlisten for connections on specified address\n"
	"-p [port number]\tlisten for connections on specified port\n"
	"-u [username]\t\tworker process be running with privileges of this user\n"
	"-r [path]\t\tchange pidfile path from default to [path]\n"
	"\n"
	, app_name);
}

int parse_opts(int argc, char **argv)
{
	int opt;
	while ((opt = getopt(argc, argv, "ha:p:u:r:")) != -1)
	{
		switch (opt)
		{	
			case 'h':
				usage(argv[0]);
				exit(0);
			case 'u':
			{
				struct passwd *pwd = NULL;
				errno = 0;
				if ((pwd = getpwnam(optarg)) == NULL)
				{
					ERROR("Can not get uid for user %s from *system* passwd database: %s\n", optarg, errno == 0 ? "not found" : strerror(errno));
					return -1;
				}
				rfsd_config.worker_uid = pwd->pw_uid;
				break;
			}
			case 'a':
				rfsd_config.listen_address = strdup(optarg);
				break;
			case 'p':
				rfsd_config.listen_port = atoi(optarg);
				break;
			case 'r':
				rfsd_config.pid_file = strdup(optarg);
				break;
			default:
				return -1;
		}
	}

	return 0;
}

int main(int argc, char **argv)
{
	init_config();

	if (parse_opts(argc, argv) != 0)
	{
		exit(1);
	}
	
	DEBUG("worker's uid: %u\n", rfsd_config.worker_uid);

	if (parse_exports() != 0)
	{
		ERROR("%s\n", "Error parsing exports file");
		return 1;
	}

#ifdef RFS_DEBUG
	dump_exports();
#endif
	
	if (load_passwords() != 0)
	{
		ERROR("%s\n", "Error loading passwords");
		release_exports();
		return 1;
	}
	
#ifdef RFS_DEBUG
	dump_passwords();
#endif

#ifndef RFS_DEBUG
	if (fork() != 0)
	{
		return 0;	}
#endif 

	if (create_pidfile(rfsd_config.pid_file) != 0)
	{
		ERROR("Error creating pidfile: %s\n", rfsd_config.pid_file);
		return 1;
	}

	int ret = start_server(rfsd_config.listen_address, rfsd_config.listen_port);
	
	release_exports();
	release_passwords();
	mp_force_free();
	
	return ret;
}
