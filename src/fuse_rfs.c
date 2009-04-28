/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include "fuse_rfs.h" /* need this before config.h because of O_ASYNC defined in compat.h */
#include "config.h"
#include "operations_rfs.h"
#include "operations_sync.h"

struct rfs_instance *instance = NULL;

struct fuse_operations fuse_rfs_operations = {
	.init		= fuse_rfs_init,
	.destroy	= fuse_rfs_destroy,

	.getattr	= fuse_rfs_getattr,
	.readdir	= fuse_rfs_readdir,
	.mkdir		= fuse_rfs_mkdir,
	.unlink		= fuse_rfs_unlink,
	.rmdir		= fuse_rfs_rmdir,
	.rename		= fuse_rfs_rename,
	.utime		= fuse_rfs_utime,
	.mknod		= fuse_rfs_mknod, /* regular files only */
	.open 		= fuse_rfs_open,
	.release	= fuse_rfs_release,
	.read 		= fuse_rfs_read,
	.write		= fuse_rfs_write,
	.truncate	= fuse_rfs_truncate,
	.flush		= fuse_rfs_flush,
	.statfs		= fuse_rfs_statfs,
	.chmod		= fuse_rfs_chmod,
	.chown		= fuse_rfs_chown,
#if FUSE_USE_VERSION >= 26
	.lock		= fuse_rfs_lock,
#endif
	.link		= fuse_rfs_link,
	.symlink	= fuse_rfs_symlink,
	.readlink	= fuse_rfs_readlink,
#if defined WITH_ACL
	.setxattr	= fuse_rfs_setxattr,
	.getxattr	= fuse_rfs_getxattr,
#endif
};

#define FUSE_DECORATE(func, instance, args...)        \
	return (func)((instance), args)

#if FUSE_USE_VERSION >= 26
void* fuse_rfs_init(struct fuse_conn_info *conn)
#else
void* fuse_rfs_init(void)
#endif
{
	return rfs_init(instance);
}

void fuse_rfs_destroy(void *init_result)
{
	rfs_destroy(instance);
}

int fuse_rfs_getattr(const char *path, struct stat *stbuf)
{
	FUSE_DECORATE(rfs_getattr, instance, path, stbuf);
}

struct readdir_data
{
	void *buf;
	const fuse_fill_dir_t filler;
};

static int fuse_rfs_readdir_callback(const char *entry_name, void *void_readdir_data)
{
	const struct readdir_data *data = (const struct readdir_data *)(void_readdir_data);
	return (data->filler(data->buf, entry_name, NULL, 0) == 0 ? 0 : -1);
}

int fuse_rfs_readdir(const char *path, void *buf, const fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
	struct readdir_data data = { buf, filler };

	FUSE_DECORATE(rfs_readdir, instance, path, fuse_rfs_readdir_callback, &data);
}

int fuse_rfs_mknod(const char *path, mode_t mode, dev_t dev)
{
	FUSE_DECORATE(rfs_mknod, instance, path, mode, dev);
}

int fuse_rfs_mkdir(const char *path, mode_t mode)
{
	FUSE_DECORATE(rfs_mkdir, instance, path, mode);
}

int fuse_rfs_unlink(const char *path)
{
	FUSE_DECORATE(rfs_unlink, instance, path);
}

int fuse_rfs_rmdir(const char *path)
{
	FUSE_DECORATE(rfs_rmdir, instance, path);
}

int fuse_rfs_rename(const char *path, const char *new_path)
{
	FUSE_DECORATE(rfs_rename, instance, path, new_path);
}

int fuse_rfs_utime(const char *path, struct utimbuf *buf)
{
	FUSE_DECORATE(rfs_utime, instance, path, buf);
}

int fuse_rfs_open(const char *path, struct fuse_file_info *fi)
{
	FUSE_DECORATE(rfs_open, instance, path, fi->flags, &fi->fh);
}

int fuse_rfs_release(const char *path, struct fuse_file_info *fi)
{
	FUSE_DECORATE(rfs_release, instance, path, fi->fh);
}

int fuse_rfs_truncate(const char *path, off_t offset)
{
	FUSE_DECORATE(rfs_truncate, instance, path, offset);
}

int fuse_rfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	FUSE_DECORATE(rfs_read, instance, path, buf, size, offset, fi->fh);
}

int fuse_rfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	FUSE_DECORATE(rfs_write, instance, path, buf, size, offset, fi->fh);
}

int fuse_rfs_flush(const char *path, struct fuse_file_info *fi)
{
	FUSE_DECORATE(rfs_flush, instance, path, fi->fh);
}

int fuse_rfs_statfs(const char *path, struct statvfs *buf)
{
	FUSE_DECORATE(rfs_statfs, instance, path, buf);
}

int fuse_rfs_chmod(const char *path, mode_t mode)
{
	FUSE_DECORATE(rfs_chmod, instance, path, mode);
}

int fuse_rfs_chown(const char *path, uid_t uid, gid_t gid)
{
	FUSE_DECORATE(rfs_chown, instance, path, uid, gid);
}

#if FUSE_USE_VERSION >= 26
int fuse_rfs_lock(const char *path, struct fuse_file_info *fi, int cmd, struct flock *fl)
{
	FUSE_DECORATE(rfs_lock, instance, path, fi->fh, cmd, fl);
}
#endif

int fuse_rfs_link(const char *path, const char *target)
{
	FUSE_DECORATE(rfs_link, instance, path, target);
}

int fuse_rfs_symlink(const char *path, const char *target)
{
	FUSE_DECORATE(rfs_symlink, instance, path, target);
}

int fuse_rfs_readlink(const char *path, char *buffer, size_t size)
{
	FUSE_DECORATE(rfs_readlink, instance, path, buffer, size);
}

#if defined WITH_ACL
int fuse_rfs_getxattr(const char *path, const char *name, char *value, size_t size)
{
	FUSE_DECORATE(rfs_getxattr, instance, path, name, value, size);
}

int fuse_rfs_setxattr(const char *path, const char *name, const char *value, size_t size, int flags)
{
	FUSE_DECORATE(rfs_setxattr, instance, path, name, value, size, flags);
}
#endif

#undef FUSE_DECORATE

