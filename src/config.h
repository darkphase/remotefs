#ifndef CONFIG_H
#define CONFIG_H

#define SERVER_PORT 5001

#ifdef RFS_DEBUG
#include <stdio.h>
#define DEBUG(format, args...) do { printf(format, args); } while (0)
#else
#define DEBUG(format, args...)
#endif

#define ERROR(format, args...) do { printf(format, args); } while (0)
#define INFO(format, args...) do { printf(format, args); } while (0)

#ifndef NULL
#define NULL (void *)(0)
#endif

//#define KEEP_ALIVE_PERIOD 60 * 5 // secs
#define KEEP_ALIVE_PERIOD 1 // secs

struct rfs_config
{
	char *host;
	char *path;
	char *auth_user;
	char *auth_passwd_file;
	char *auth_passwd;
};

enum
{
	KEY_HELP,
};

enum rfs_open_flags
{
	RFS_APPEND 		= 1, 
	RFS_ASYNC 		= 2,
	RFS_CREAT 		= 4,
	RFS_EXCL 		= 8,
	RFS_NONBLOCK 	= 16,
	RFS_NDELAY 		= 32,
	RFS_SYNC 		= 64,
	RFS_TRUNC 		= 128,
	RFS_RDONLY		= 512,
	RFS_WRONLY		= 1024,
	RFS_RDWR		= 2048,
};

#endif // CONFIG_H
