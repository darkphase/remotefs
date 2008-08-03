#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <fuse.h>
#include <fuse_opt.h>

#include "config.h"
#include "operations.h"
#include "sendrecv.h"
#include "command.h"
#include "buffer.h"
#include "alloc.h"
#include "passwd.h"
#include "crypt.h"

struct rfs_config rfs_config = { 0 };

#ifdef RFS_DEBUG
int test_connection()
{
	struct stat ret;

	int i = 0; for (i = 0; i < 10; ++i)
	{
		int done = rfs_getattr("/", &ret);
		DEBUG("getattr returned: %d\n", done);
	}
	
	return 0;}
#endif /* RFS_DEBUG */

#define RFS_OPT(t, p, v) { t, offsetof(struct rfs_config, p), v } 

struct fuse_opt rfs_opts[] = 
{
	FUSE_OPT_KEY("-h", KEY_HELP),
	FUSE_OPT_KEY("--help", KEY_HELP),
	RFS_OPT("username=%s", auth_user, 0),
	RFS_OPT("password=%s", auth_passwd_file, 0),
	RFS_OPT("rd_cache=%u", use_read_cache, 0),
	RFS_OPT("wr_cache=%u", use_write_cache, 0),
	RFS_OPT("rdwr_cache=%u", use_read_write_cache, 0),
	RFS_OPT("port=%u", server_port, DEFAULT_SERVER_PORT),
	
	FUSE_OPT_END
};

void usage(const char *program)
{
	printf(
"usage: %s host:path mountpoint [options]\n"
"\n"
"general options:\n"
"    -o opt,[opt...]         mount options\n"
"    -h   --help             print help\n"
"\n"
"RFS options:\n"
"    -o username=name        auth username\n"
"    -o rd_cache=0           disable read cache\n"
"    -o wr_cache=0           disable write cache\n"
"    -o rdwr_cache=0         disable read/write cache\n"
"    -o password=filename    filename with password for auth\n"
"    -o port=server_port     port which the server is listening to\n"
"\n"
"\n", 
	program);
}

void init_config()
{
	rfs_config.server_port = DEFAULT_SERVER_PORT;
	rfs_config.use_read_cache = 1;
	rfs_config.use_write_cache = 1;
	rfs_config.use_read_write_cache = 1;
}

void rfs_fix_options()
{
	if (rfs_config.use_read_write_cache == 0)
	{
		rfs_config.use_read_cache = 0;
		rfs_config.use_write_cache = 0;
	}
}

int rfs_opt_proc(void *data, const char *arg, int key, struct fuse_args *outargs)
{
	switch (key)
	{	
	case FUSE_OPT_KEY_NONOPT:
		if (rfs_config.host == NULL 
		&& rfs_config.path == NULL)
		{
			const char *delimiter = strchr(arg, ':');
			if (delimiter != NULL)
			{
				rfs_config.host = get_buffer(delimiter - arg + 1);
				memset(rfs_config.host, 0, delimiter - arg + 1);
				memcpy(rfs_config.host, arg, delimiter - arg);
				
				rfs_config.path = get_buffer(strlen(arg) - (delimiter - arg));
				memset(rfs_config.path, 0, strlen(arg) - (delimiter - arg));
				memcpy(rfs_config.path, delimiter + 1, strlen(arg) - (delimiter - arg) - 1);
				
				return 0;
			}
		}
		return 1;
	
	case KEY_HELP:
		usage(outargs->argv[0]);
		fuse_opt_add_arg(outargs, "-ho");
		fuse_main(outargs->argc, outargs->argv, (struct fuse_operations *)NULL);
		exit(0);
	}
	
	return 1;
}

int read_password()
{
	if (rfs_config.auth_passwd_file == NULL)
	{
		return -EINVAL;
	}

	errno = 0;
	FILE *fp = fopen(rfs_config.auth_passwd_file, "rt");
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
	buffer[done - 1] == '\n')
	{
		--done;
		buffer[done] = 0;
	}
	
	rfs_config.auth_passwd = passwd_hash(buffer, EMPTY_SALT);
	DEBUG("hashed passwd: %s\n", rfs_config.auth_passwd);
	free_buffer(buffer);
	
	return 0;
}

int main(int argc, char **argv)
{
	init_config();
	
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	
	if (fuse_opt_parse(&args, &rfs_config, rfs_opts, rfs_opt_proc) == -1)
	{
		exit(1);
	}
	
	fuse_opt_add_arg(&args, "-s");
	++argc;
	
	rfs_fix_options();
	
	if (rfs_config.host == NULL)
	{
		ERROR("%s\n", "Remote host is not specified");
		exit(1);
	}
	
	if (rfs_config.path == NULL)
	{
		ERROR("%s\n", "Remote path is not specified");
		exit(1);
	}
	
#ifdef RFS_DEBUG
	if (rfs_config.use_read_cache != 0)
	{
		DEBUG("%s\n", "using read cache");
	}
	
	if (rfs_config.use_write_cache != 0)
	{
		DEBUG("%s\n", "using write cache");
	}
#endif /* RFS_DEBUG */
	
	if (rfs_config.auth_user != NULL
	&& rfs_config.auth_passwd_file != NULL)
	{
		int read_ret = read_password();
		if (read_ret != 0)
		{
			ERROR("Error reading password file: %s\n", strerror(-read_ret));
			return 1;
		}
	}
	
	DEBUG("username: %s, password: %s\n", rfs_config.auth_user, rfs_config.auth_passwd);
	
	if (rfs_reconnect(1) != 0)
	{
		return 1;
	}

	int ret = fuse_main(args.argc, args.argv, &rfs_operations);
	/*	int ret = test_connection(); */

	fuse_opt_free_args(&args);

	free_buffer(rfs_config.host);
	free_buffer(rfs_config.path);
	
	if (rfs_config.auth_passwd != NULL)
	{
		free(rfs_config.auth_passwd);
	}
	
	mp_force_free();
	
	return ret;
}
