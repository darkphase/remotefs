/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <sys/socket.h>
#include <arpa/inet.h>
#if defined FREEBSD || defined QNX
#       include <netinet/in.h>
#endif
#ifdef WITH_IPV6
#       include <netdb.h>
#endif
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>

#include "config.h"
#include "rfsd.h"
#include "signals_server.h"
#include "exports.h"
#include "passwd.h"
#include "keep_alive_server.h"
#include "sockets.h"
#include "sug_server.h"
#include "instance.h"
#include "server.h"

static int daemonize = 1;
static int listen_socket = -1;

static DEFINE_RFSD_INSTANCE(rfsd_instance);

static int create_pidfile(const char *pidfile)
{
	FILE *fp = fopen(pidfile, "wt");
	if (fp == NULL)
	{
		return -1;
	}
	
	if (fprintf(fp, "%d", getpid()) < 1)
	{
		fclose(fp);
		return -1;
	}
	
	fclose(fp);
	
	return 0;
}

static int start_server(const char *address, const unsigned port)
{
	install_signal_handlers_server();
	
	int listen_family = AF_INET;
	
	struct sockaddr_in addr = { 0 };
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	inet_pton(AF_INET, address, &(addr.sin_addr));
	
#if defined WITH_IPV6
	struct sockaddr_in6 addr6 = { 0 };
	addr6.sin6_family = AF_INET6;
	addr6.sin6_port = htons(port);
	if (inet_pton(AF_INET6, address, &(addr6.sin6_addr)) == 1)
	{
		listen_family = AF_INET6;
	}
#endif
	
	errno = 0;
	if (listen_family == AF_INET)
	{
		listen_socket = socket(PF_INET, SOCK_STREAM, 0);
	}
#if defined WITH_IPV6
	else
	{
		listen_socket = socket(PF_INET6, SOCK_STREAM, 0);
	}
#endif
	if (listen_socket == -1)
	{
		ERROR("Error creating socket: %s\n", strerror(errno));
		return 1;
	}

	errno = 0;
	if (setup_socket_reuse(listen_socket, 1) != 0)
	{
		ERROR("Error setting proper option to socket: %s\n", strerror(errno));
		return 1;
	}
	
	errno = 0;
	if (listen_family == AF_INET)
	{
		if (bind(listen_socket, (struct sockaddr*)&addr, sizeof(addr)) == -1)
		{
			ERROR("Error binding: %s\n", strerror(errno));
			return 1;
		}
	}
#ifdef WITH_IPV6
	else
	{
		if (bind(listen_socket,(struct sockaddr*) &addr6, sizeof(addr6)) == -1)
		{
			ERROR("Error binding: %s\n", strerror(errno));
			return 1;
		}
	}
#endif

	if (listen(listen_socket, 10) == -1)
	{
		ERROR("Error listening: %s\n", strerror(errno));
		return 1;
	}
	
#ifdef RFS_DEBUG
	if (listen_family == AF_INET)
	{
		char straddr[INET_ADDRSTRLEN + 1] = { 0 };
		DEBUG("listening on %s (%d)\n", 
		inet_ntop(AF_INET, &addr.sin_addr, straddr, sizeof(straddr)), 
		port);
	}
#	ifdef WITH_IPV6
	else
	{
		char straddr[INET6_ADDRSTRLEN + 1] = { 0 };
		DEBUG("listening on %s (%d)\n", 
		inet_ntop(AF_INET6, &addr6.sin6_addr, straddr, sizeof(straddr)), 
		port);
	}
#	endif
#endif
	
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
	
	while (1)
	{
#ifndef WITH_IPV6
		struct sockaddr_in client_addr;
#else
		struct sockaddr_storage client_addr;
#endif
		socklen_t addr_len = sizeof(client_addr);
		int client_socket = accept(listen_socket, (struct sockaddr *)&client_addr, &addr_len);
		if (client_socket == -1)
		{
			continue;
		}
		
		if (((struct sockaddr_in *)&client_addr)->sin_family == AF_INET)
		{
			DEBUG("incoming connection from %s\n", inet_ntoa(((struct sockaddr_in*)&client_addr)->sin_addr));
		}
#ifdef WITH_IPV6
		else
		{
			char straddr[INET6_ADDRSTRLEN + 1] = { 0 };
			inet_ntop(AF_INET6, &((struct sockaddr_in6*)&client_addr)->sin6_addr, straddr, sizeof(straddr));
			DEBUG("incoming connection from %s\n",straddr);
		}
#endif

		if (fork() == 0) /* child */
		{
			return handle_connection(&rfsd_instance, client_socket, &client_addr);
		}
		else
		{
			close(client_socket);
		}
	}
}

void stop_server()
{
	server_close_connection(&rfsd_instance);
	unlink(rfsd_instance.config.pid_file);
	release_rfsd_instance(&rfsd_instance);
	
	exit(0);
}

void check_keep_alive()
{
	if (keep_alive_locked(&rfsd_instance) != 0
	&& keep_alive_expired(&rfsd_instance) == 0)
	{
		DEBUG("%s\n", "keep alive expired");
		server_close_connection(&rfsd_instance);
		exit(1);
	}
	
	alarm(keep_alive_period());
}

static void usage(const char *app_name)
{
	printf("usage: %s [options]\n"
	"\n"
	"Options:\n"
	"-h \t\t\tshow this help screen\n"
	"-a [address]\t\tlisten to connections on [address]\n"
	"-p [port number]\tlisten to connections on [port number]\n"
	"-u [username]\t\trun worker process with privileges of [username]\n"
	"-g [groupname]\t\trun worker process with privileges of [groupname]\n"
	"-r [path]\t\tchange pidfile path from default to [path]\n"
	"-f \t\t\tstay foreground\n"
	"-e [path]\t\texports file\n"
	"-s [path]\t\tpasswd file\n"
	"-q \t\t\tquite mode - supress warnings\n"
	"\t\t\t(and don't treat them as errors)\n"
	"\n"
	, app_name);
}

static int parse_opts(int argc, char **argv)
{
	int opt;
	while ((opt = getopt(argc, argv, "hqa:p:u:g:r:e:s:f")) != -1)
	{
		switch (opt)
		{	
			case 'h':
				usage(argv[0]);
				release_rfsd_instance(&rfsd_instance);
				exit(0);
			case 'q':
				rfsd_instance.config.quiet = 1;
				break;
			case 'u':
			{
				struct passwd *pwd = getpwnam(optarg);
				if (pwd == NULL)
				{
					ERROR("Can not get uid for user %s from *system* passwd database: %s\n", optarg, errno == 0 ? "not found" : strerror(errno));
					return -1;
				}
				rfsd_instance.config.worker_uid = pwd->pw_uid;
				break;
			}
			case 'g':
			{
				struct group *grp = getgrnam(optarg);
				if (grp == NULL)
				{
					ERROR("Can not get gid for group %s from *system* passwd database: %s\n", optarg, errno == 0 ? "not found" : strerror(errno));
					return -1;
				}
				rfsd_instance.config.worker_gid = grp->gr_gid;
				break;
			}
			case 'a':
				free(rfsd_instance.config.listen_address);
				rfsd_instance.config.listen_address = strdup(optarg);
				break;
			case 'p':
				rfsd_instance.config.listen_port = atoi(optarg);
				break;
			case 'r':
				free(rfsd_instance.config.pid_file);
				rfsd_instance.config.pid_file = strdup(optarg);
				break;
			case 'e':
				free(rfsd_instance.config.exports_file);
				rfsd_instance.config.exports_file = strdup(optarg);
				break;
			case 's':
				free(rfsd_instance.config.passwd_file);
				rfsd_instance.config.passwd_file = strdup(optarg);
				break;
			case 'f':
				daemonize = 0;
				break;
			default:
				return -1;
		}
	}

	return 0;
}

int main(int argc, char **argv)
{
	init_rfsd_instance(&rfsd_instance);
	
	if (parse_opts(argc, argv) != 0)
	{
		exit(1);
	}

	if (parse_exports(rfsd_instance.config.exports_file, 
	&rfsd_instance.exports.list, 
	rfsd_instance.config.worker_uid, 
	rfsd_instance.config.worker_gid) != 0)
	{
		ERROR("Error parsing exports file at %s\n", rfsd_instance.config.exports_file);
		return 1;
	}
	
#ifdef RFS_DEBUG
	dump_exports(rfsd_instance.exports.list);
#endif
	
	DEBUG("loading passwords from %s\n", rfsd_instance.config.passwd_file);
	if (load_passwords(rfsd_instance.config.passwd_file, &rfsd_instance.passwd.auths) != 0)
	{
		ERROR("Error loading passwords from %s\n", rfsd_instance.config.passwd_file);
		return 1;
	}
	
#ifdef RFS_DEBUG
	dump_passwords(rfsd_instance.passwd.auths);
#endif

	if (rfsd_instance.config.quiet == 0) /* don't do checks, 
	since they won't be displayed anyway */
	{
		if (suggest_server(&rfsd_instance) != 0)
		{
			ERROR("%s\n", "ERROR: Warning considered as error, exiting. (use -q to disable this)");
			return 1;
		}
	}

#ifndef RFS_DEBUG
	if (daemonize != 0 && fork() != 0)
	{
		return 0;
	}
#endif 

	if (create_pidfile(rfsd_instance.config.pid_file) != 0)
	{
		ERROR("Error creating pidfile: %s\n", rfsd_instance.config.pid_file);
		return 1;
	}

	int ret = start_server(rfsd_instance.config.listen_address, rfsd_instance.config.listen_port);
	
	release_exports(&rfsd_instance.exports.list);
	release_passwords(&rfsd_instance.passwd.auths);
	release_rfsd_instance(&rfsd_instance);
	
	return ret;
}
