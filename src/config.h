/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef CONFIG_H
#define CONFIG_H

/** rfs configuration */

#include <stdio.h>
#include <sys/types.h>

#include "compat.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#define DEFAULT_SERVER_PORT 	5001
#define KEEP_ALIVE_PERIOD 	60 * 5      /* secs */
#define DEFAULT_RW_CACHE_SIZE 	512 * 1024  /* bytes */
#define ATTR_CACHE_TTL 		5           /* secs */
#define MAX_SALT_LEN 		3 + 8       /* "$1$" + 8 bytes of actual salt */
#define EMPTY_SALT 		"$1$"       /* use md5 */
#define MAX_SUPPORTED_NAME_LEN  32          /* if username or group is longer than 32 
					    characters, then this user should be insane */
#define RFS_WRITE_BLOCK         32 * 1024   /* bytes */
#define RFS_READ_BLOCK          32 * 1024   /* bytes */

#ifdef RFS_DEBUG
#        define DEBUG(format, args...) do { printf(format, args); } while (0)
#else
#        define DEBUG(format, args...) {}
#endif

#define ERROR(format, args...) do { fprintf(stderr, format, args); } while (0)
#define INFO(format, args...) do { fprintf(stdout, format, args); } while (0)
#define WARN(format, args...) do { if (rfs_config.quiet == 0) { fprintf(stdout, format, args); } } while (0)

#ifndef NULL
#        define NULL (void *)(0)
#endif

/** client options */
struct rfs_config
{
	char *host;
	char *path;
	char *auth_user;
	char *auth_passwd_file;
	char *auth_passwd;
	unsigned int use_write_cache;
	unsigned int use_read_cache;
	unsigned int use_read_write_cache;
	unsigned int server_port;
	unsigned char quiet;
};

/** server options */
struct rfsd_config
{
	const char *listen_address;
	const char *pid_file;
	unsigned int listen_port;
	uid_t worker_uid;
	gid_t worker_gid;
};

enum
{
	KEY_HELP,
	KEY_QUIET
};

/** on/off export options */
enum rfs_export_opts { opt_none = 0, opt_ro = 1, opt_ugo = 2, opt_compat = 3 };

/** flags for rfs_open() */
enum rfs_open_flags
{
	RFS_APPEND 		= 1, 
	RFS_ASYNC 		= 2,
	RFS_CREAT 		= 4,
	RFS_EXCL 		= 8,
	RFS_NONBLOCK 		= 16,
	RFS_NDELAY 		= 32,
	RFS_SYNC 		= 64,
	RFS_TRUNC 		= 128,
	RFS_RDONLY		= 512,
	RFS_WRONLY		= 1024,
	RFS_RDWR		= 2048
};

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* CONFIG_H */
