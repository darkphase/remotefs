#ifndef CONFIG_H
#define CONFIG_H

/** rfs configuration */

#include <stdio.h>
#include <sys/types.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#define DEFAULT_SERVER_PORT 	5001
#define KEEP_ALIVE_PERIOD 	60 * 5 /* secs */
#define DEFAULT_RW_CACHE_SIZE 	1024 * 1024 /* bytes */
#define CACHE_TTL 		20 /* secs */
#define MAX_SALT_LEN 		3 + 8 /* "$1$" + 8 bytes of actual salt */
#define EMPTY_SALT 		"$1$" /* use md5 */

#ifdef RFS_DEBUG
#        define DEBUG(format, args...) do { printf(format, args); } while (0)
#else
#        define DEBUG(format, args...) {}
#endif

#define ERROR(format, args...) do { printf(format, args); } while (0)
#define INFO(format, args...) do { printf(format, args); } while (0)

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
};

/** server options */
struct rfsd_config
{
	const char *listen_address;
	const char *pid_file;
	unsigned int listen_port;
	uid_t worker_uid;
};

enum
{
	KEY_HELP
};

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
