/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "client_common.h"
#include "client_for_server.h"
#include "config.h"
#include "global_lock.h"
#include "server.h"

static char *rfs_host = NULL;
static int inc_value = 0;

static void usage(const char *app)
{
	printf("Usage: %s -r <rfs_host> <-s | -k> [-a] [-f]\n", app);
}

static int parse_opts(int argc, char **argv, struct config *config)
{
	const char *avail_opts = "hr:skaf";
	int opt = 0;

	while ((opt = getopt(argc, argv, avail_opts)) != -1)
	{
		switch (opt)
		{
			case 'f':
				config->fork = 0;
				break;
			case 'a':
				config->allow_other = 1;
				break;
			case 'k':
				if (inc_value != 0)
				{
					return -1;
				}
				inc_value = -1;
				break;
			case 's':
				if (inc_value != 0)
				{
					return -1;
				}
				inc_value = 1;
				break;
			case 'r':
				rfs_host = strdup(optarg);
				break;
			case 'h':
				usage(argv[0]);
				exit(1);
			default:
				return -1;
		}
	}

	return 0;
}

int main(int argc, char **argv)
{
	struct config server_config = { 0 };
	init_config(&server_config);

	if (parse_opts(argc, argv, &server_config) != 0)
	{
		usage(argv[0]);
		exit(1);
	}

	if (rfs_host == NULL
	|| inc_value == 0)
	{
		usage(argv[0]);
		exit(1);
	}

	uid_t myuid = (server_config.allow_other ? (uid_t)-1 : getuid());

	if (get_global_lock(&server_config) != 0)
	{
		release_global_lock(&server_config);
		return 1;
	}

	/* start server */
	if (inc_value == 1)
	{		
		unsigned running = rfsnss_is_server_running(myuid);
		
		if (running == 0)
		{
			if (start_listen(myuid, &server_config) != 0)
			{
				goto start_error;
			}

			/* child block */
			if (fork() == 0) /* if we're going to fork (normally in release setup)
			then we need to start accepting connections in background 
			and quit after rfs server is added */
			{
				if (server_config.fork != 0)
				{
					if (start_accepting(&server_config) != 0)
					{
						goto start_error;
					}
				}
				else
				{	
					if (rfsnss_addserver(myuid, rfs_host) != 0)
					{
						goto add_error;
					}
				}

				return 0;
			}
			else /* otherwise, if we want to see debug output (or something) in foreground
			then we need to add rfs server in background 
			and accept connections in foreground */
			{
				if (server_config.fork != 0)
				{
					if (rfsnss_addserver(myuid, rfs_host) != 0)
					{
						goto add_error;
					}
				}
				else
				{
					if (start_accepting(&server_config) != 0)
					{
						goto start_error;
					}
				}

				/* note that global lock will be locked till server stops,
				so only one server in debug */
				release_global_lock(&server_config);
			}
		
			return 0;
		}
		else /* corresponding server is already running - 
		just add new users and groups to it */
		{
			int add_ret = rfsnss_addserver(myuid, rfs_host);

			release_global_lock(&server_config);
			return (add_ret == 0 ? 0 : 1);
		}
	}
	else if (inc_value == -1) /* try to stop running server */
	{
		int dec_ret = rfsnss_dec(myuid);
		release_global_lock(&server_config);

		return (dec_ret == 0 ? 0 : 1);
	}

	release_global_lock(&server_config);
	return 0;

start_error:
	release_global_lock(&server_config);
	stop_server(&server_config, 1);

add_error:

	return 1;
}

