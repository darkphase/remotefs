/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>

#include "config.h"
#include "buffer.h"
#include "passwd.h"
#include "crypt.h"
#include "id_lookup.h"
#include "sug_client.h"
#include "instance_client.h"
#include "operations_rfs.h"
#include "fuse_rfs.h"

static DEFINE_RFS_INSTANCE(rfs_instance);

#ifdef WITH_EXPORTS_LIST
static unsigned just_list_exports = 0;
#endif

#define RFS_OPT(t, p, v) { t, offsetof(struct rfs_config, p), v } 

struct fuse_opt rfs_opts[] = 
{
	FUSE_OPT_KEY("-h", KEY_HELP),
	FUSE_OPT_KEY("-q", KEY_QUIET),
#ifdef WITH_EXPORTS_LIST
	FUSE_OPT_KEY("-l", KEY_LISTEXPORTS),
#endif
	FUSE_OPT_KEY("--help", KEY_HELP),
	RFS_OPT("username=%s", auth_user, 0),
	RFS_OPT("password=%s", auth_passwd_file, 0),
	RFS_OPT("rd_cache=%u", use_read_cache, 0),
	RFS_OPT("wr_cache=%u", use_write_cache, 0),
	RFS_OPT("rdwr_cache=%u", use_read_write_cache, 0),
	RFS_OPT("port=%u", server_port, DEFAULT_SERVER_PORT),
	RFS_OPT("socket_timeout=%d", socket_timeout, -1),
	RFS_OPT("socket_buffer=%d", socket_buffer, -1),
#ifdef WITH_SSL
	RFS_OPT("ssl", enable_ssl, 1),
	RFS_OPT("ciphers=%s", ssl_ciphers, 0),
#endif
	
	FUSE_OPT_END
};

static void usage(const char *program)
{
	printf(
	"usage: %s host:path mountpoint [options]\n"
#ifdef WITH_IPV6
	"enclose IPv6 address with [] brackets\n"
#endif
	"\n"
	"\n"
	"general options:\n"
	"    -o opt,[opt...]         mount options\n"
	"    -h   --help             print help\n"
	"\n"
	"RFS options:\n"
	"    -q                      suppress warnings\n"
	"    -l                      list exports of specified host (and exit)\n"
	"    -o username=name        auth username\n"
	"    -o rd_cache=0           disable read cache\n"
	"    -o wr_cache=0           disable write cache\n"
	"    -o rdwr_cache=0         disable read/write cache\n"
	"    -o password=filename    filename with password for auth\n"
	"    -o port=server_port     port which the server is listening to\n"
#ifdef WITH_SSL
	"    -o ssl                  enable SSL\n"
#endif
	"\n"
	"\n", 
	program);
}

static void rfs_fix_options()
{
	if (rfs_instance.config.use_read_write_cache == 0)
	{
		rfs_instance.config.use_read_cache = 0;
		rfs_instance.config.use_write_cache = 0;
	}
}

static int rfs_opt_proc(void *data, const char *arg, int key, struct fuse_args *outargs)
{
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
					return 1;
				}
				
				rfs_instance.config.host = get_buffer(delimiter - arg);
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
					rfs_instance.config.host = get_buffer(delimiter - arg + 1);
					memset(rfs_instance.config.host, 0, delimiter - arg + 1);
					memcpy(rfs_instance.config.host, arg, delimiter - arg);
				}
				else
				{
					rfs_instance.config.host = get_buffer(strlen(arg) + 1);
					memcpy(rfs_instance.config.host, arg, strlen(arg) + 1);
				}
#if WITH_IPV6
			}
#endif
			
			if (delimiter != NULL)
			{
				rfs_instance.config.path = get_buffer(strlen(arg) - (delimiter - arg));
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
	
	case KEY_QUIET:
		rfs_instance.config.quiet = 1;
		return 0;
	
#ifdef WITH_EXPORTS_LIST
	case KEY_LISTEXPORTS:
		just_list_exports = 1;
		return 0;
#endif
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
	
	char *buffer = get_buffer(size + 1);
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
	free_buffer(buffer);
	
	return 0;
}

#ifdef WITH_EXPORTS_LIST
int list_exports_main()
{
	int conn_ret = rfs_reconnect(&rfs_instance, 1, 0);
	if (conn_ret != 0)
	{
		return -conn_ret;
	}
	
	int list_ret = rfs_list_exports(&rfs_instance);
	if (list_ret < 0)
	{
		ERROR("Error listing exports: %s\n", strerror(-list_ret));
	}
	
	rfs_disconnect(&rfs_instance, 1);
	
	free_buffer(rfs_instance.config.host);
	
	if (rfs_instance.config.path != 0)
	{
		free_buffer(rfs_instance.config.path);
	}
	
	if (rfs_instance.config.auth_passwd != NULL)
	{
		free(rfs_instance.config.auth_passwd);
	}
	
	return list_ret;
}
#endif

int main(int argc, char **argv)
{
	init_rfs_instance(&rfs_instance);
	instance = &rfs_instance;

	/* for the cache utility */
	umask(0600);
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	
	if (fuse_opt_parse(&args, &rfs_instance.config, rfs_opts, rfs_opt_proc) == -1)
	{
		exit(1);
	}
	
	fuse_opt_add_arg(&args, "-s");
	++argc;
	
	rfs_fix_options();
	
	if (rfs_instance.config.host == NULL)
	{
		ERROR("%s\n", "Remote host is not specified");
		fuse_opt_free_args(&args);
		exit(1);
	}
	
#ifdef WITH_EXPORTS_LIST
	if (just_list_exports != 0)
	{
		exit(list_exports_main());
	}
#endif
	
	if (rfs_instance.config.path == NULL)
	{
		ERROR("%s\n", "Remote path is not specified");
		exit(1);
	}
	
#ifdef RFS_DEBUG
	if (rfs_instance.config.use_read_cache != 0)
	{
		DEBUG("%s\n", "using read cache");
	}
	
	if (rfs_instance.config.use_write_cache != 0)
	{
		DEBUG("%s\n", "using write cache");
	}
	
	DEBUG("socket timeout: %d\n", rfs_instance.config.socket_timeout);
	DEBUG("socket buffer: %d\n", rfs_instance.config.socket_buffer);
#endif /* RFS_DEBUG */
	
	if (rfs_instance.config.auth_user != NULL
	&& rfs_instance.config.auth_passwd_file != NULL)
	{
		int read_ret = read_password();
		if (read_ret != 0)
		{
			ERROR("Error reading password file: %s\n", strerror(-read_ret));
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
		return 1;
	}
	
	char opt_tmpl[] = "-ofsname=rfs@";
	size_t fsname_len = strlen(opt_tmpl) + strlen(rfs_instance.config.host) + 1;
	char *opt = get_buffer(fsname_len);
	memset(opt, 0, fsname_len);
	memcpy(opt, opt_tmpl, strlen(opt_tmpl));
	memcpy(opt + strlen(opt_tmpl), rfs_instance.config.host, strlen(rfs_instance.config.host));

	fuse_opt_add_arg(&args, opt);
	++argc;

#if FUSE_USE_VERSION >= 26
	int ret = fuse_main(args.argc, args.argv, &fuse_rfs_operations, NULL);
#else
	int ret = fuse_main(args.argc, args.argv, &fuse_rfs_operations);
#endif

	rfs_disconnect(&rfs_instance, 0);
	
	fuse_opt_free_args(&args);

	free_buffer(opt);

	free_buffer(rfs_instance.config.host);
	free_buffer(rfs_instance.config.path);
	
	if (rfs_instance.config.auth_passwd != NULL)
	{
		free(rfs_instance.config.auth_passwd);
	}
	
	destroy_uids_lookup(&rfs_instance.id_lookup.uids);
	destroy_gids_lookup(&rfs_instance.id_lookup.gids);
	release_rfs_instance(&rfs_instance);
	
	return ret;
}
