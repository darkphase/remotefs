/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_SYNC_H
#define OPERATIONS_SYNC_H

/** rfs synced operations */

#include <sys/types.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct flock;
struct rfs_instance;

typedef int(*rfs_readdir_callback_t)(const char *, void *);

/* dirs */
int rfs_mkdir(struct rfs_instance *instance, const char *path, mode_t mode);
int rfs_rmdir(struct rfs_instance *instance, const char *path);
int rfs_readdir(struct rfs_instance *instance, const char *path, const rfs_readdir_callback_t callback, void *callback_data);

/* files */
int rfs_getattr(struct rfs_instance *instance, const char *path, struct stat *stbuf);
int rfs_mknod(struct rfs_instance *instance, const char *path, mode_t mode, dev_t dev);
int rfs_unlink(struct rfs_instance *instance, const char *path);
int rfs_rename(struct rfs_instance *instance, const char *path, const char *new_path);
int rfs_utime(struct rfs_instance *instance, const char *path, struct utimbuf *buf);
int rfs_truncate(struct rfs_instance *instance, const char *path, off_t offset);
int rfs_chmod(struct rfs_instance *instance, const char *path, mode_t mode);
int rfs_chown(struct rfs_instance *instance, const char *path, uid_t uid, gid_t gid);

/* i/o */
int rfs_open(struct rfs_instance *instance, const char *path, int flags, uint64_t *desc);
int rfs_release(struct rfs_instance *instance, const char *path, uint64_t desc);
int rfs_read(struct rfs_instance *instance, const char *path, char *buf, size_t size, off_t offset, uint64_t desc);
int rfs_write(struct rfs_instance *instance, const char *path, const char *buf, size_t size, off_t offset, uint64_t desc);
int rfs_flush(struct rfs_instance *instance, const char *path, uint64_t desc);
int rfs_lock(struct rfs_instance *instance, const char *path, uint64_t desc, int cmd, struct flock *fl);

/* fs */
int rfs_statfs(struct rfs_instance *instance, const char *path, struct statvfs *buf);

#if defined WITH_LINKS
/* links */
int rfs_link(struct rfs_instance *instance, const char *path, const char *target);
int rfs_symlink(struct rfs_instance *instance, const char *path, const char *target);
int rfs_readlink(struct rfs_instance *instance, const char *path, char *buffer, size_t size);
#endif

#if defined WITH_ACL
/* acl */
int rfs_getxattr(struct rfs_instance *instance, const char *path, const char *name, char *value, size_t size);
int rfs_setxattr(struct rfs_instance *instance, const char *path, const char *name, const char *value, size_t size, int flags);
#endif

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* OPERATIONS_SYNC_H */
