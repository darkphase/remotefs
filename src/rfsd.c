/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <unistd.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>

#include "config.h"
#include "exports.h"
#include "instance_server.h"
#include "keep_alive_server.h"
#include "list.h"
#include "options.h"
#include "passwd.h"
#include "rfsd.h"
#ifdef SCHEDULING_AVAILABLE
#	include "scheduling.h"
#endif
#include "server.h"
#include "signals_server.h"
#include "sockets.h"
#include "sug_server.h"
#include "version.h"

static int daemonize = 1;

static DEFINE_RFSD_INSTANCE(rfsd_instance);

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

static void release_server(struct rfsd_instance *instance)
{
	release_exports(&rfsd_instance.exports.list);
	release_passwords(&rfsd_instance.passwd.auths);

	unlink(rfsd_instance.config.pid_file);
	release_rfsd_instance(&rfsd_instance);
}

static int start_server(const struct list *addresses, const unsigned port, unsigned force_ipv4, unsigned force_ipv6)
{
	install_signal_handlers_server();

	int listen_sockets[MAX_LISTEN_ADDRESSES];
	memset(listen_sockets, -1, sizeof(listen_sockets) / sizeof(listen_sockets[0]));
	unsigned listen_number = 0;
	int max_socket_number = -1;

	const struct list *address_item = addresses;
	while (address_item != NULL)
	{
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

		listen_sockets[listen_number] = listen_socket;
		++listen_number;
		max_socket_number = (max_socket_number < listen_socket ? listen_socket : max_socket_number);

		errno = 0;
		if (setup_socket_reuse(listen_socket, 1) != 0)
		{
			ERROR("Error setting reuse option for %s: %s\n", address, strerror(errno));
			return 1;
		}

#if defined WITH_IPV6
		if (force_ipv6 != 0)
		{
			if (setup_socket_ipv6_only(listen_socket) != 0)
			{
				ERROR("Error setting IPv6-only option for %s: %s\n", address, strerror(errno));
				return 1;
			}
		}
#endif

		if (listen_family == AF_INET)
		{
			errno = 0;
			if (bind(listen_socket, (struct sockaddr*)&addr, sizeof(addr)) != 0)
			{
				ERROR("Error binding to %s: %s\n", address, strerror(errno));
				return 1;
			}

		}
#ifdef WITH_IPV6
		else if (listen_family == AF_INET6)
		{			
			errno = 0;
			if (bind(listen_socket, (struct sockaddr*)&addr6, sizeof(addr6)) != 0)
			{
				ERROR("Error binding to %s: %s\n", address, strerror(errno));
				return 1;
			}
		}
#endif

		if (listen(listen_socket, LISTEN_BACKLOG) != 0)
		{
			ERROR("Error listening to %s: %s\n", address, strerror(errno));
			return 1;
		}

#ifdef WITH_IPV6
		DEBUG("listening to %s interface: %s (%d)\n", listen_family == AF_INET ? "IPv4" : "IPv6", address, port);
#else
		DEBUG("listening to IPv4 interface: %s (%d)\n", address, port);
#endif
			
		address_item = address_item->next;
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
				
				struct sockaddr_in client_addr;
				socklen_t addr_len = sizeof(client_addr);

				int client_socket = accept(listen_socket, (struct sockaddr *)&client_addr, &addr_len);
				if (client_socket == -1)
				{
					continue;
				}
		
#ifdef RFS_DEBUG
				char straddr[256] = { 0 };
#ifdef WITH_IPV6
				inet_ntop(client_addr.sin_family, 
				client_addr.sin_family == AF_INET 
					? (const void *)&((struct sockaddr_in*)&client_addr)->sin_addr 
					: (const void *)&((struct sockaddr_in6*)&client_addr)->sin6_addr, 
				straddr, sizeof(straddr));
#else
				inet_ntop(AF_INET, &((struct sockaddr_in*)&client_addr)->sin_addr, straddr, sizeof(straddr));
#endif

				DEBUG("incoming connection from %s\n",straddr);
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

					return handle_connection(&rfsd_instance, client_socket, &client_addr);
				}
				else
				{
					close(client_socket);
				}		
			}
		}
	}
}

void stop_server()
{
	server_close_connection(&rfsd_instance);
	release_server(&rfsd_instance);

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
	"-h                  print help\n"
	"-v                  print version and build date and time\n"
	"-a [address]        listen to connections on [address]\n"
	"-p [port number]    listen to connections on [port number]\n"
	"-u [username]       run worker process with privileges of [username]\n"
	"-r [path]           change pidfile path from default to [path]\n"
	"-f                  stay foreground\n"
	"-e [path]           exports file\n"
	"-s [path]           passwd file\n"
	"-q                  quite mode - supress warnings\n"
	"                    (and don't treat them as errors)\n"
#ifdef WITH_IPV6
	"-4                  force listen to IPv4 connections\n"
	"-6                  force listen to IPv6 connections\n"
#endif
	"\n"
	, app_name);
}

static int parse_opts(int argc, char **argv)
{
	int opt;
#ifdef WITH_IPV6
	while ((opt = getopt(argc, argv, "vhqa:p:u:r:e:s:f46")) != -1)
#else
	while ((opt = getopt(argc, argv, "vhqa:p:u:r:e:s:f")) != -1)
#endif
	{
		switch (opt)
		{	
			case 'h':
				usage(argv[0]);
				release_rfsd_instance(&rfsd_instance);
				exit(0);
			case 'v':
				INFO("%s\n", RFS_FULL_VERSION);
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
			case 'a':
				destroy_list(&rfsd_instance.config.listen_addresses);
				rfsd_instance.config.listen_addresses = parse_list(optarg, optarg + strlen(optarg));
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
#ifdef WITH_IPV6
			case '4':
				rfsd_instance.config.force_ipv4 = 1;
				if (list_length(rfsd_instance.config.listen_addresses) == 1 
				&& strcmp(rfsd_instance.config.listen_addresses->data, DEFAULT_IPV6_ADDRESS) == 0)
				{
					destroy_list(&rfsd_instance.config.listen_addresses);
					add_to_list(&rfsd_instance.config.listen_addresses, strdup(DEFAULT_IPV4_ADDRESS));
				}
				break;
			case '6':
				rfsd_instance.config.force_ipv6 = 1;
				if (list_length(rfsd_instance.config.listen_addresses) == 1 
				&& strcmp(rfsd_instance.config.listen_addresses->data, DEFAULT_IPV4_ADDRESS) == 0)
				{
					destroy_list(&rfsd_instance.config.listen_addresses);
					add_to_list(&rfsd_instance.config.listen_addresses, strdup(DEFAULT_IPV6_ADDRESS));
				}
				break;
#endif
			default:
				return -1;
		}
	}

	return 0;
}

static int validate_config(const struct rfsd_config *config)
{
	if (list_length(config->listen_addresses) > MAX_LISTEN_ADDRESSES)
	{
		ERROR("Maximum number of %d listen addresses is allowed\n", MAX_LISTEN_ADDRESSES);
		return -1;
	}

#ifdef WITH_IPV6
	struct list *item = config->listen_addresses;
	while (item != NULL)
	{
		if (config->force_ipv4 
		&& strchr(item->data, ':') != NULL)
		{
			ERROR("You can't use IPv6 addresses (like %s) with -4 option\n", (const char *)item->data);
			return -1;
		}
	
		if (config->force_ipv6 != 0 
		&& strchr(item->data, ':') == NULL)
		{
			ERROR("You can't use IPv4 addresses (like %s) with -6 option\n", (const char *)item->data);
			return -1;
		}

		item = item->next;
	}
#endif

	return 0;
}

int main(int argc, char **argv)
{
	init_rfsd_instance(&rfsd_instance);
	
	if (parse_opts(argc, argv) != 0)
	{
		release_rfsd_instance(&rfsd_instance);
		exit(1);
	}

	if (validate_config(&rfsd_instance.config) != 0)
	{
		release_rfsd_instance(&rfsd_instance);
		exit(1);
	}

	int parse_ret = parse_exports(rfsd_instance.config.exports_file, 
	&rfsd_instance.exports.list, 
	rfsd_instance.config.worker_uid);

	if (parse_ret > 0)
	{
		ERROR("Error parsing exports file at %s (line %d)\n", rfsd_instance.config.exports_file, parse_ret);
		release_server(&rfsd_instance);
		exit(1);
	}
	else if (parse_ret < 0)
	{
		ERROR("Error parsing exports file at %s: %s\n", rfsd_instance.config.exports_file, strerror(-parse_ret));
		release_server(&rfsd_instance);
		exit(1);
	}
	
#ifdef RFS_DEBUG
	dump_exports(rfsd_instance.exports.list);
#endif
	
	if (load_passwords(rfsd_instance.config.passwd_file, &rfsd_instance.passwd.auths) != 0)
	{
		ERROR("Error loading passwords from %s\n", rfsd_instance.config.passwd_file);
		release_server(&rfsd_instance);
		exit(1);
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
			release_server(&rfsd_instance);
			exit(1);
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
		release_server(&rfsd_instance);
		exit(1);
	}

	int ret = start_server(rfsd_instance.config.listen_addresses, 
		rfsd_instance.config.listen_port, 
#ifdef WITH_IPV6
		rfsd_instance.config.force_ipv4, rfsd_instance.config.force_ipv6
#else
		1, 0
#endif
		);
	
	release_server(&rfsd_instance);
	
	return ret;
}

