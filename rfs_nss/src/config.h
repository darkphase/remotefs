/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef RFSNSS_CONFIG_H
#define RFSNSS_CONFIG_H

/** rfs_nss configuration */

#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct rfs_list;

#define COOKIES_TTL             10 * 60 /* secs */
#define ZERO_CONNECTIONS_TTL    1 * 60 /* secs*/

#define SOCKETS_PATTERN         "/tmp/rfs_nssd-%d"
#define SHARED_SOCKET           "/tmp/rfs_nssd-shared"
#define GLOBAL_LOCK             "/tmp/rfs_nssd.lock"
#define START_UID               10000
#define START_GID               START_UID
#define RFS_NSS_MAX_NAME        1024
#define DEFAULT_SHELL           "/bin/false"
#define DEFAULT_HOME            "/tmp"
#define DEFAULT_PASSWD          "+"

#ifndef NULL
#	define NULL (void *)(0)
#endif

#ifdef RFS_DEBUG
#define DEBUG(format, ...) do { fprintf(stderr, format, ## __VA_ARGS__); } while (0)
#else
#define DEBUG(format, ...)
#endif

struct user_info
{
	char *name;
	uid_t uid;
};

struct group_info
{
	char *name;
	gid_t gid;
};

struct config
{
	unsigned connections;
	time_t connections_updated;

	int sock;
	char *socketname;
	unsigned fork;
	unsigned allow_other;

	int lock;

	uid_t last_uid;
	gid_t last_gid;

	struct rfs_list *users;
	struct rfs_list *groups;

	void *user_cookies;
	void *group_cookies;

	pthread_t maintenance_thread;
	pthread_mutex_t maintenance_lock;
	unsigned stop_maintenance_thread;
};

void init_config(struct config *config);
void release_config(struct config *config);
void release_users(struct rfs_list **users);
void release_groups(struct rfs_list **groups);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* RFSNSS_CONFIG_H */

