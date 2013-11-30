/*
remotefs file system
See the file AUTHORS for copyright information.

This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>

#include "fuse_rfs.h" /* need this before config.h because of O_ASYNC defined in compat.h */
#include "buffer.h"
#include "config.h"
#include "crypt.h"
#include "id_lookup.h"
#include "instance_client.h"
#ifdef WITH_EXPORTS_LIST
# 	include "list_exports.h"
#endif
#include "operations/operations_rfs.h"
#include "passwd.h"
#include "sug_client.h"
#include "version.h"

static DEFINE_RFS_INSTANCE(rfs_instance);

static unsigned just_list_exports = 0;

/** remotefs client parse options flags */
enum
{
	KEY_HELP
	, KEY_VERSION
	, KEY_QUIET
	, KEY_LISTEXPORTS
#if defined WITH_IPV6
	, KEY_IPV4
	, KEY_IPV6
#endif
};

#define RFS_OPT(t, p, v) { t, offsetof(struct rfs_config, p), v }

struct fuse_opt rfs_opts[] =
{
	FUSE_OPT_KEY("-h", KEY_HELP),
	FUSE_OPT_KEY("-v", KEY_VERSION),
	FUSE_OPT_KEY("-q", KEY_QUIET),
	FUSE_OPT_KEY("-l", KEY_LISTEXPORTS),
#if defined WITH_IPV6
	FUSE_OPT_KEY("-4", KEY_IPV4),
	FUSE_OPT_KEY("-6", KEY_IPV6),
#endif
	FUSE_OPT_KEY("--help", KEY_HELP),
	RFS_OPT("username=%s", auth_user, 0),
	RFS_OPT("password=%s", auth_passwd_file, 0),
	RFS_OPT("port=%u", server_port, DEFAULT_SERVER_PORT),
	RFS_OPT("transform_symlinks", transform_symlinks, 1),

	FUSE_OPT_END
};

static void usage(const char *program)
{
	fprintf(stderr,
	"usage: %s host:path mountpoint [options]\n"
#ifdef WITH_IPV6
	"enclose IPv6 address with [] brackets\n"
#endif
	"\n"
	"\n"
	"general options:\n"
	"    -o opt,[opt...]         mount options\n"
	"    -h   --help             print help\n"
	"    -v                      print version and quit\n"
	"\n"
	"RFS options:\n"
	"    -q                      suppress warnings\n"
	"    -4                      force use of IPv4\n"
	"    -6                      force use of IPv6\n"
#ifdef WITH_EXPORTS_LIST
	"    -l                      list exports of specified host (and exit)\n"
#endif
	"    -o port=server_port     port which the server is listening to\n"
	"    -o username=name        auth username\n"
	"    -o password=filename    filename with password for auth\n"
	"    -o transform_symlinks   transform absolute symlinks to relative\n"
	"\n"
	"\n",
	program);
}

static int rfs_opt_proc(void *data, const char *arg, int key, struct fuse_args *outargs)
{
	DEBUG("opt passed: '%s'\n", arg);

	if (strcmp(arg, "allow_other") == 0)
	{
		rfs_instance.config.allow_other = 1;
	}

	if (strstr(arg, "fsname=") == arg)
	{
		rfs_instance.config.set_fsname = 0;
	}

	switch (key)
	{
	case FUSE_OPT_KEY_NONOPT:
		if (rfs_instance.config.host == NULL
		&& rfs_instance.config.path == NULL)
		{
			const char *delimiter = NULL;
#ifdef WITH_IPV6
			/* if we have an IPv6 address we enclose it within [ ] */
			if ( *arg == '[' )
			{
				delimiter = strchr(arg, ']');
				if (delimiter == NULL)
				{
					ERROR("%s\n", "No matching ']' in IPv6 host");
					exit(1);
				}

				rfs_instance.config.host = malloc(delimiter - arg);
				memset(rfs_instance.config.host, 0, delimiter - arg);
				memcpy(rfs_instance.config.host, arg + 1, delimiter - arg - 1);

				delimiter = (delimiter[1] == ':' ? delimiter + 1 : NULL);
			}
			else
			{
#endif
				delimiter = strchr(arg, ':');

				if (delimiter != NULL)
				{
					rfs_instance.config.host = malloc(delimiter - arg + 1);
					memset(rfs_instance.config.host, 0, delimiter - arg + 1);
					memcpy(rfs_instance.config.host, arg, delimiter - arg);
				}
				else
				{
					rfs_instance.config.host = malloc(strlen(arg) + 1);
					memcpy(rfs_instance.config.host, arg, strlen(arg) + 1);
				}
#if WITH_IPV6
			}
#endif

			if (delimiter != NULL)
			{
				rfs_instance.config.path = malloc(strlen(arg) - (delimiter - arg));
				memset(rfs_instance.config.path, 0, strlen(arg) - (delimiter - arg));
				memcpy(rfs_instance.config.path, delimiter + 1, strlen(arg) - (delimiter - arg) - 1);
			}

			return 0;
		}
		return 1;

	case KEY_HELP:
		usage(outargs->argv[0]);
		fuse_opt_add_arg(outargs, "-ho");
#if FUSE_USE_VERSION >= 26
		fuse_main(outargs->argc, outargs->argv, (struct fuse_operations *)NULL, NULL);
#else
		fuse_main(outargs->argc, outargs->argv, (struct fuse_operations *)NULL);
#endif
		exit(0);

	case KEY_VERSION:
		INFO("%s\n", RFS_FULL_VERSION);
		exit(0);
	case KEY_QUIET:
		rfs_instance.config.quiet = 1;
		return 0;
#if defined WITH_IPV6
	case KEY_IPV4:
		((struct rfs_config*)data)->force_ipv4 = 1;
		return 0;

	case KEY_IPV6:
		((struct rfs_config*)data)->force_ipv6 = 1;
		return 0;
#endif

	case KEY_LISTEXPORTS:
		just_list_exports = 1;
		return 0;
	}

	return 1;
}

static int read_password()
{
	if (rfs_instance.config.auth_passwd_file == NULL)
	{
		return -EINVAL;
	}

	errno = 0;
	FILE *fp = fopen(rfs_instance.config.auth_passwd_file, "rt");
	if (fp == NULL)
	{
		return -errno;
	}

	errno = 0;
	if (fseek(fp, 0, SEEK_END) != 0)
	{
		int saved_errno = errno;
		fclose(fp);
		return -saved_errno;
	}

	errno = 0;
	long size = ftell(fp);

	if (size == -1)
	{
		int saved_errno = errno;
		fclose(fp);
		return -saved_errno;
	}

	errno = 0;
	if (fseek(fp, 0, SEEK_SET) != 0)
	{
		int saved_errno = errno;
		fclose(fp);
		return -saved_errno;
	}

	char *buffer = malloc(size + 1);
	memset(buffer, 0, size + 1);

	int done = fread(buffer, 1, size, fp);

	if (done != size)
	{
		fclose(fp);
		return -EIO;
	}

	fclose(fp);

	while (done > 0 &&
	(buffer[done - 1] == '\n'
	|| buffer[done - 1] == '\r'
	|| buffer[done - 1] == '\t'
	|| buffer[done - 1] == ' '))
	{
		--done;
		buffer[done] = 0;
	}

	rfs_instance.config.auth_passwd = passwd_hash(buffer, EMPTY_SALT);
	DEBUG("hashed passwd: %s\n", rfs_instance.config.auth_passwd ? rfs_instance.config.auth_passwd : "NULL");
	free(buffer);

	return 0;
}

int main(int argc, char **argv)
{
	init_rfs_instance(&rfs_instance);
	instance = &rfs_instance;

	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	if (fuse_opt_parse(&args, &rfs_instance.config, rfs_opts, rfs_opt_proc) == -1)
	{
		exit(1);
	}

	fuse_opt_add_arg(&args, "-s");
	++argc;

	fuse_opt_add_arg(&args, "-obig_writes");
	++argc;

	if (rfs_instance.config.host == NULL)
	{
		ERROR("%s\n", "Remote host is not specified");
		fuse_opt_free_args(&args);
		exit(1);
	}

#ifdef WITH_EXPORTS_LIST
	if (just_list_exports != 0)
	{
		fuse_opt_free_args(&args);
		exit(-list_exports_main(&rfs_instance));
	}
#else
	if (just_list_exports != 0)
	{
		ERROR("%s\n\n", "Exports list isn't supported in this build of remotefs");

		fuse_opt_free_args(&args);
		usage(argv[0]);
		exit(1);
	}
#endif

	if (rfs_instance.config.path == NULL)
	{
		ERROR("%s\n", "Remote path is not specified");
		fuse_opt_free_args(&args);
		exit(1);
	}

#ifdef RFS_DEBUG
	if (rfs_instance.config.transform_symlinks != 0)
	{
		DEBUG("%s\n", "transforming symlinks");
	}

	if (rfs_instance.config.allow_other != 0)
	{
		DEBUG("%s\n", "sharing mount with other users");
	}
#endif /* RFS_DEBUG */

	if (rfs_instance.config.auth_user != NULL
	&& rfs_instance.config.auth_passwd_file != NULL)
	{
		int read_ret = read_password();
		if (read_ret != 0)
		{
			ERROR("Error reading password file: %s\n", strerror(-read_ret));
			fuse_opt_free_args(&args);
			return 1;
		}
	}

	DEBUG("username: %s, password: %s\n",
	rfs_instance.config.auth_user ? rfs_instance.config.auth_user : "NULL",
	rfs_instance.config.auth_passwd ? rfs_instance.config.auth_passwd : "NULL");

	if (rfs_instance.config.quiet == 0)
	{
		suggest_client(&rfs_instance);
	}

	if (rfs_reconnect(&rfs_instance, 1, 1) != 0)
	{
		fuse_opt_free_args(&args);
		return 1;
	}

	DEFINE_FUSE_RFS_INSTANCE(fuse_rfs_instance);
	init_fuse_rfs_instance(&fuse_rfs_instance);

	setup_fsname(&fuse_rfs_instance, &rfs_instance, &argc, &args);

#if FUSE_USE_VERSION >= 26
	int ret = fuse_main(args.argc, args.argv, &fuse_rfs_operations, NULL);
#else
	int ret = fuse_main(args.argc, args.argv, &fuse_rfs_operations);
#endif

	rfs_disconnect(&rfs_instance, 0);

	fuse_opt_free_args(&args);

	free(rfs_instance.config.host);
	free(rfs_instance.config.path);

	if (rfs_instance.config.auth_passwd != NULL)
	{
		free(rfs_instance.config.auth_passwd);
	}

	release_fuse_rfs_instance(&fuse_rfs_instance);
	release_rfs_instance(&rfs_instance);

	return ret;
}

