/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef CONFIG_H
#define CONFIG_H

/** remotefs configuration */

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

#include "compat.h"
#include "options.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct list;

#define DEFAULT_SERVER_PORT     5001
#define KEEP_ALIVE_PERIOD       60 * 5      /* secs */
#define DEFAULT_RW_CACHE_SIZE   512 * 1024  /* bytes */
#define ATTR_CACHE_TTL          5           /* secs */
#define ATTR_CACHE_MAX_ENTRIES  10000
#define EMPTY_SALT              "$1$"       /* use md5 */
#define MAX_SALT_LEN            3 + 8       /* "$1$" + 8 bytes of actual salt */
#define MAX_SUPPORTED_NAME_LEN  32          /* used in chown(), getattr() and etc related to stat() */
#define RFS_WRITE_BLOCK         32 * 1024   /* bytes */
#define RFS_READ_BLOCK          32 * 1024   /* bytes */
#define RFS_APPROX_READ_BLOCK 	256 * 1024  /* bytes */
#define RFS_MAX_READ_BLOCK 		4096 * 1024 /* bytes */
#define SENDFILE_THRESHOLD      4 * 1024    /* bytes */
#define DEFAULT_IPV4_ADDRESS    "0.0.0.0"
#ifdef WITH_IPV6
#define DEFAULT_IPV6_ADDRESS    "::"
#endif
#define LISTEN_BACKLOG          10
#define MAX_LISTEN_ADDRESSES    16
#define ALL_ACCESS_USERNAME     "*"

#define STAT_BLOCK_SIZE sizeof(uint32_t) /* mode */ \
	+ sizeof(uint64_t) /* size */                   \
	+ sizeof(uint64_t) /* atime */                  \
	+ sizeof(uint64_t) /* mtime */                  \
	+ sizeof(uint64_t) /* ctime */                  \
	+ sizeof(uint32_t) /* nlink */                  \
	+ sizeof(uint32_t) /* blocks */                 \

#ifdef RFS_DEBUG
#define DEFAULT_PASSWD_FILE      "./rfs-passwd"
#define DEFAULT_EXPORTS_FILE     "./rfs-exports"
#define DEFAULT_PID_FILE         "./rfsd.pid"
#else
#define DEFAULT_PASSWD_FILE      "/etc/rfs-passwd"
#define DEFAULT_EXPORTS_FILE     "/etc/rfs-exports"
#define DEFAULT_PID_FILE         "/var/run/rfsd.pid"
#endif /* RFS_DEBUG */

#ifdef RFS_DEBUG
#define RFS_NSS_BIN             "LD_LIBRARY_PATH=. ./rfs_nssd"
#else
#define RFS_NSS_BIN             "rfs_nssd"
#endif
#define NSS_SOCKETS_DIR         "/tmp/"
#define RFS_NSS_RFSHOST_OPTION  "-r"
#define RFS_NSS_START_OPTION    "-s"
#define RFS_NSS_STOP_OPTION     "-k"
#define RFS_NSS_SHARED_OPTION   "-a"

#ifdef RFS_DEBUG
#        define DEBUG(format, args...) do { fprintf(stderr, "%u: ", (unsigned)getpid()); fprintf(stderr, format, args); } while (0)
#else
#        define DEBUG(format, args...) {}
#endif

#define ERROR(format, args...) do { fprintf(stderr, format, args); } while (0)
#define INFO(format, args...) do { fprintf(stdout, format, args); } while (0)
#define WARN(format, args...) do { fprintf(stdout, format, args); } while (0)

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
	unsigned int server_port;
	unsigned int quiet;
	unsigned transform_symlinks;
	unsigned allow_other;
	unsigned set_fsname;
#ifdef WITH_IPV6
	unsigned force_ipv4;
	unsigned force_ipv6;
#endif
};

/** server options */
struct rfsd_config
{
	struct list* listen_addresses;
	char *pid_file;
	unsigned int listen_port;
	uid_t worker_uid;
	unsigned int quiet;
	char *exports_file;
	char *passwd_file;
#ifdef WITH_IPV6
	unsigned force_ipv4;
	unsigned force_ipv6;
#endif
};

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* CONFIG_H */
