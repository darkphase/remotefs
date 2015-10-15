/*
remotefs file system
See the file AUTHORS for copyright information.

This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <assert.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

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
#include "sug_server.h"
#include "version.h"

static int daemonize = 1;
DEFINE_RFSD_INSTANCE(rfsd_instance);

static void usage(const char *app_name)
{
	assert(DEFAULT_SEND_TIMEOUT == DEFAULT_RECV_TIMEOUT);

	printf("usage: %s [options]\n"
	"\n"
	"Options:\n"
	"-h                  print help\n"
	"-v                  print version and quit\n"
	"-a [address]        listen to connections on [address]\n"
	"-p [port number]    listen to connections on [port number]\n"
	"-u [username]       run worker process with privileges of [username]\n"
	"-r [path]           change pidfile path from default to [path]\n"
	"-f                  stay foreground\n"
	"-e [path]           exports file\n"
	"-s [path]           passwd file\n"
	"-t [timeout]        timeouts for send-recv operations on sockets in seconds (default: %d)\n"
	"-q                  quite mode - supress warnings\n"
	"                    (and don't treat them as errors)\n"
	"\n"
	, app_name, DEFAULT_SEND_TIMEOUT / 1000000);
}

static int parse_opts(int argc, char **argv)
{
	int opt;
#ifdef WITH_IPV6
	while ((opt = getopt(argc, argv, "vhqa:p:u:r:e:s:t:f46")) != -1)
#else
	while ((opt = getopt(argc, argv, "vhqa:p:u:r:e:s:t:f")) != -1)
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
			case 't':
			{
				int timeouts = atoi(optarg);
				if (timeouts > 0)
				{
					rfsd_instance.sendrecv.recv_timeout = timeouts * 1000000; /* in useconds */
					rfsd_instance.sendrecv.send_timeout = timeouts * 1000000;
				}
				break;
			}
#ifdef WITH_IPV6
			case '4':
				WARN("%s\n", "WARNING: -4 option is deprecated. Use -a with an IPv4 address instead.");
				break;
			case '6':
				WARN("%s\n", "WARNING: -6 option is deprecated. Use -a with an IPv6 address instead.");
				break;
#endif
			default:
				return -1;
		}
	}

	return 0;
}

static int validate_config(const rfsd_config_t *config)
{
	if (list_length(config->listen_addresses) > MAX_LISTEN_ADDRESSES)
	{
		ERROR("Exceeded maximum number (%d) of listen addresses\n", MAX_LISTEN_ADDRESSES);
		return -1;
	}

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

	install_signal_handlers_server();

	int ret = start_server(&rfsd_instance, daemonize);

	release_server(&rfsd_instance);
	return ret;
}
