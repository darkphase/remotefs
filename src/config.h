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

#include "compat.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#define DEFAULT_SERVER_PORT     5001
#define KEEP_ALIVE_PERIOD       60 * 5      /* secs */
#define DEFAULT_RW_CACHE_SIZE   512 * 1024  /* bytes */
#define ATTR_CACHE_TTL          5           /* secs */
#define EMPTY_SALT              "$1$"       /* use md5 */
#define MAX_SALT_LEN            3 + 8       /* "$1$" + 8 bytes of actual salt */
#define MAX_SUPPORTED_NAME_LEN  32          /* used in chown(), getattr() and etc related to stat() */
#define RFS_WRITE_BLOCK         32 * 1024   /* bytes */
#define RFS_READ_BLOCK          32 * 1024   /* bytes */
#define PREFETCH_LIMIT          32 * 1024   /* bytes */
#define SENDFILE_LIMIT          4 * 1024    /* bytes */
#define SSL_READ_BLOCK          8 * 1024    /* bytes */
#define SSL_WRITE_BLOCK         8 * 1024    /* bytes */
#define RFS_DEFAULT_CIPHERS     "RC4-MD5:AES128-MD5:RC4:AES128:ALL:@STRENGTH"

#ifdef RFS_DEBUG
#define DEFAULT_PASSWD_FILE      "./rfs-passwd"
#define DEFAULT_EXPORTS_FILE     "./rfs-exports"
#define DEFAULT_PID_FILE         "./rfsd.pid"
#define DEFAULT_CLIENT_KEY_FILE  "rfs-key.pem"
#define DEFAULT_CLIENT_CERT_FILE "rfs-cert.pem"
#define DEFAULT_SERVER_KEY_FILE  DEFAULT_CLIENT_KEY_FILE 
#define DEFAULT_SERVER_CERT_FILE DEFAULT_CLIENT_CERT_FILE
#else
#define DEFAULT_PASSWD_FILE      "/etc/rfs-passwd"
#define DEFAULT_EXPORTS_FILE     "/etc/rfs-exports"
#define DEFAULT_PID_FILE         "/var/run/rfsd.pid"
#define DEFAULT_CLIENT_KEY_FILE  ".rfs/rfs-key.pem"    /* client's files will be concatenated with $HOME */
#define DEFAULT_CLIENT_CERT_FILE ".rfs/rfs-cert.pem"
#define DEFAULT_SERVER_KEY_FILE  "/etc/rfsd-key.pem"
#define DEFAULT_SERVER_CERT_FILE "/etc/rfsd-cert.pem"
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
#        define DEBUG(format, args...) do { printf(format, args); } while (0)
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
#ifdef WITH_SSL
	unsigned int enable_ssl;
	char *ssl_key_file;
	char *ssl_cert_file;
	char *ssl_ciphers;
#endif
};

/** server options */
struct rfsd_config
{
	char *listen_address;
	char *pid_file;
	unsigned int listen_port;
	uid_t worker_uid;
	gid_t worker_gid;
	unsigned int quiet;
	char *exports_file;
	char *passwd_file;
#ifdef WITH_SSL
	char *ssl_key_file;
	char *ssl_cert_file;
	char *ssl_ciphers;
#endif
};

/** remotefs client parse options flags */
enum
{
	KEY_HELP
	, KEY_QUIET
#ifdef WITH_EXPORTS_LIST 
	, KEY_LISTEXPORTS
#endif
};

/** on/off export options */
enum rfs_export_opts 
{ 
	OPT_NONE         = 0, 
	OPT_RO           = 1, 
#ifdef WITH_UGO
	OPT_UGO          = 2, 
#else
	RESERVER_FOR_UGO = 2,
#endif
	
	/* reserved */
	
	OPT_COMPAT       = 50
};

/** flags for rfs_open() */
enum rfs_open_flags
{
	RFS_APPEND 		= 1, 
	RFS_ASYNC 		= 2,
	RFS_CREAT 		= 4,
	RFS_EXCL 		= 8,
	RFS_NONBLOCK    = 16,
	RFS_NDELAY 		= 32,
	RFS_SYNC 		= 64,
	RFS_TRUNC 		= 128,
	RFS_RDONLY		= 512,
	RFS_WRONLY		= 1024,
	RFS_RDWR		= 2048
};

/** flags for rfs_lock() */
enum rfs_lock_flags
{
	RFS_GETLK               = 1,
	RFS_SETLK               = 2,
	RFS_SETLKW              = 4
};

/** types for rfs_lock() */
enum rfs_lock_type
{
	RFS_RDLCK               = 1,
	RFS_WRLCK               = 2,
	RFS_UNLCK               = 4
};

#ifdef WITH_ACL
enum rfs_acl_flags
{
	RFS_XATTR_CREATE          = 1,
	RFS_XATTR_REPLACE         = 2
};
#endif

/** write-behind control struct */
struct write_behind_request
{
	struct cache_block *block;
	unsigned int please_die;
	int last_ret;
	char *path;
};

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* CONFIG_H */

