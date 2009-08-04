/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef FUSE_RFS_H
#define FUSE_RFS_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#include <fuse.h>

struct rfs_instance;

extern struct rfs_instance *instance;
struct fuse_operations fuse_rfs_operations;

#if FUSE_USE_VERSION >= 26
void* fuse_rfs_init(struct fuse_conn_info *conn);
#else
void* fuse_rfs_init(void);
#endif
void fuse_rfs_destroy(void *init_result);

int fuse_rfs_getattr(const char *path, struct stat *stbuf);
int fuse_rfs_readdir(const char *path, void *buf, const fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
int fuse_rfs_mknod(const char *path, mode_t mode, dev_t dev);
int fuse_rfs_mkdir(const char *path, mode_t mode);
int fuse_rfs_unlink(const char *path);
int fuse_rfs_rmdir(const char *path);
int fuse_rfs_rename(const char *path, const char *new_path);
#if FUSE_USE_VERSION < 26
int fuse_rfs_utime(const char *path, struct utimbuf *buf);
#endif
#if FUSE_USE_VERSION >= 26
int fuse_rfs_utimens(const char *path, const struct timespec tv[2]);
#endif
int fuse_rfs_mknod(const char *path, mode_t mode, dev_t dev);
int fuse_rfs_create(const char *path, mode_t mode, struct fuse_file_info *fi);
int fuse_rfs_open(const char *path, struct fuse_file_info *fi);
int fuse_rfs_release(const char *path, struct fuse_file_info *fi);
int fuse_rfs_truncate(const char *path, off_t offset);
int fuse_rfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int fuse_rfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int fuse_rfs_flush(const char *path, struct fuse_file_info *fi);
int fuse_rfs_statfs(const char *path, struct statvfs *buf);
int fuse_rfs_chmod(const char *path, mode_t mode);
int fuse_rfs_chown(const char *path, uid_t uid, gid_t gid);
#if FUSE_USE_VERSION >= 26
int fuse_rfs_lock(const char *path, struct fuse_file_info *fi, int cmd, struct flock *fl);
#endif
int fuse_rfs_link(const char *path, const char *target);
int fuse_rfs_symlink(const char *path, const char *target);
int fuse_rfs_readlink(const char *path, char *buffer, size_t size);
#if defined WITH_ACL
int fuse_rfs_getxattr(const char *path, const char *name, char *value, size_t size);
int fuse_rfs_setxattr(const char *path, const char *name, const char *vslue, size_t size, int flags);
#endif

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* FUSE_RFS_H */

