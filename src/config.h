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

struct rfs_list;

#define DEFAULT_SERVER_PORT     5001
#define KEEP_ALIVE_PERIOD       60 * 5      /* secs */
#define DEFAULT_RW_CACHE_SIZE   512 * 1024  /* bytes */
#define ATTR_CACHE_TTL          20          /* secs */
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
#define MAX_LISTEN_ADDRESSES    16
#define LISTEN_BACKLOG          MAX_LISTEN_ADDRESSES
#define ALL_ACCESS_USERNAME     "*"
#define DEFAULT_RECV_TIMEOUT	15 * 1000000     /* usecs */
#define DEFAULT_SEND_TIMEOUT	15 * 1000000     /* usecs */
#define DEFAULT_CONNECT_TIMEOUT	1000000     /* usecs */

#define STAT_BLOCK_SIZE sizeof(uint32_t) /* mode */ \
	+ sizeof(uint64_t) /* size */                   \
	+ sizeof(uint64_t) /* atime */                  \
	+ sizeof(uint64_t) /* mtime */                  \
	+ sizeof(uint64_t) /* ctime */                  \
	+ sizeof(uint32_t) /* nlink */                  \
	+ sizeof(uint32_t) /* blocks */                 \
	+ sizeof(uint64_t) /* ino */                    \

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
#        define DEBUG(format, ...) do { fprintf(stderr, "%u: ", (unsigned)getpid()); fprintf(stderr, format, ## __VA_ARGS__); } while (0)
#else
#        define DEBUG(format, ...)
#endif

#define ERROR(format, ...) do { fprintf(stderr, format, ## __VA_ARGS__); } while (0)
#define INFO(format, ...) do { fprintf(stdout, format, ## __VA_ARGS__); } while (0)
#define WARN(format, ...) do { fprintf(stdout, format, ## __VA_ARGS__ ); } while (0)

#ifndef NULL
#        define NULL (void *)(0)
#endif

/** client options */
typedef struct
{
	char *host;
	char *path;
	char *auth_user;
	char *auth_passwd_file;
	char *auth_passwd;
	unsigned server_port;
	unsigned quiet;
	unsigned transform_symlinks;
	unsigned allow_other;
	unsigned set_fsname;
	unsigned timeouts;
	unsigned connect_timeout;
#ifdef WITH_IPV6
	unsigned force_ipv4;
	unsigned force_ipv6;
#endif
} rfs_config_t;

/** server options */
typedef struct
{
	struct rfs_list* listen_addresses;
	char *pid_file;
	unsigned int listen_port;
	uid_t worker_uid;
	unsigned int quiet;
	char *exports_file;
	char *passwd_file;
} rfsd_config_t;

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* CONFIG_H */
