/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_H
#define OPERATIONS_H

/** rfs operations */

#include "options.h"

/* if connection lost after executing the operation
and operation failed, then try it one more time 

pass ret (int) as return value of this macro */
#define PARTIALY_DECORATE(ret, func, instance, args...)     \
	if(check_connection((instance)) == 0)               \
	{                                                   \
		(ret) = (func)((instance), args);           \
		if ((ret) == -ECONNABORTED                  \
		&& check_connection((instance)) == 0)       \
		{                                           \
			(ret) = (func)((instance), args);   \
		}                                           \
	}

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct flock;
struct rfs_instance;
struct stat;
struct statvfs;
struct utimbuf;

typedef int(*rfs_readdir_callback_t)(const char *, void *);

int _rfs_getattr(struct rfs_instance *instance, const char *path, struct stat *stbuf);
int _rfs_statfs(struct rfs_instance *instance, const char *path, struct statvfs *buf);
int _rfs_utime(struct rfs_instance *instance, const char *path, struct utimbuf *buf);
int _rfs_utimens(struct rfs_instance *instance, const char *path, const struct timespec tv[2]);

/* dirs */
int _rfs_mkdir(struct rfs_instance *instance, const char *path, mode_t mode);
int _rfs_rmdir(struct rfs_instance *instance, const char *path);
int _rfs_readdir(struct rfs_instance *instance, const char *path, const rfs_readdir_callback_t callback, void *callback_data);

/* files */
int _rfs_mknod(struct rfs_instance *instance, const char *path, mode_t mode, dev_t dev);
int _rfs_create(struct rfs_instance *instance, const char *path, mode_t mode, int flags, uint64_t *desc);
int _rfs_unlink(struct rfs_instance *instance, const char *path);
int _rfs_rename(struct rfs_instance *instance, const char *path, const char *new_path);
int _rfs_truncate(struct rfs_instance *instance, const char *path, off_t offset);
int _rfs_lock(struct rfs_instance *instance, const char *path, uint64_t desc, int cmd, struct flock *fl);

/* i/o */
int _rfs_open(struct rfs_instance *instance, const char *path, int flags, uint64_t *desc);
int _rfs_release(struct rfs_instance *instance, const char *path, uint64_t desc);

/* read */
int _rfs_read(struct rfs_instance *instance, const char *path, char *buf, size_t size, off_t offset, uint64_t desc);

/* write */
int _rfs_write(struct rfs_instance *instance, const char *path, const char *buf, size_t size, off_t offset, uint64_t desc);
int _rfs_flush(struct rfs_instance *instance, const char *path, uint64_t desc);

/* access */
int _rfs_chmod(struct rfs_instance *instance, const char *path, mode_t mode);
int _rfs_chown(struct rfs_instance *instance, const char *path, uid_t uid, gid_t gid);

/* links */
int _rfs_link(struct rfs_instance *instance, const char *path, const char *target);
int _rfs_symlink(struct rfs_instance *instance, const char *path, const char *target);
int _rfs_readlink(struct rfs_instance *instance, const char *path, char *buffer, size_t size);

#if defined ACL_OPERATIONS_AVAILABLE
/* acl */
int _rfs_getxattr(struct rfs_instance *instance, const char *path, const char *name, char *value, size_t size);
int _rfs_setxattr(struct rfs_instance *instance, const char *path, const char *name, const char *value, size_t size, int flags);
#endif

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* OPERATIONS_H */

