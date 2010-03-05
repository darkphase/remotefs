/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

/** syncronized operations. will lock keep alive when it's needed */

#include <errno.h>

#include "../options.h"
#include "../instance.h"
#include "../keep_alive_client.h"
#include "operations.h"
#include "operations_rfs.h"

#define DECORATE(func, instance, args...)                       \
	int ret = -ECONNABORTED;                                    \
	if (client_keep_alive_lock((instance)) == 0)                \
	{                                                           \
		PARTIALY_DECORATE(ret, (func), (instance), args)        \
		client_keep_alive_unlock((instance));                   \
	}                                                           \
	else                                                        \
	{                                                           \
		ret = -EIO;                                             \
	}                                                           \
	return ret

int rfs_mknod(struct rfs_instance *instance, const char *path, mode_t mode, dev_t dev)
{
	DECORATE(_rfs_mknod, instance, path, mode, dev);
}

int rfs_chmod(struct rfs_instance *instance, const char *path, mode_t mode)
{
	DECORATE(_rfs_chmod, instance, path, mode);
}

int rfs_chown(struct rfs_instance *instance, const char *path, uid_t uid, gid_t gid)
{
	DECORATE(_rfs_chown, instance, path, uid, gid);
}

int rfs_statfs(struct rfs_instance *instance, const char *path, struct statvfs *buf)
{
	DECORATE(_rfs_statfs, instance, path, buf);
}

int rfs_utime(struct rfs_instance *instance, const char *path, struct utimbuf *buf)
{
	DECORATE(_rfs_utime, instance, path, buf);
}

int rfs_utimens(struct rfs_instance *instance, const char *path, const struct timespec tv[2])
{
	DECORATE(_rfs_utimens, instance, path, tv);
}

int rfs_rename(struct rfs_instance *instance, const char *path, const char *new_path)
{
	DECORATE(_rfs_rename, instance, path, new_path);
}

int rfs_rmdir(struct rfs_instance *instance, const char *path)
{
	DECORATE(_rfs_rmdir, instance, path);
}

int rfs_unlink(struct rfs_instance *instance, const char *path)
{
	DECORATE(_rfs_unlink, instance, path);
}

int rfs_mkdir(struct rfs_instance *instance, const char *path, mode_t mode)
{
	DECORATE(_rfs_mkdir, instance, path, mode);
}

int rfs_write(struct rfs_instance *instance, const char *path, const char *buf, size_t size, off_t offset, uint64_t desc)
{
	/* we have to manually handle keep alive in _rfs_write and _rfs_write_cached
	for _rfs_read_cached to be able to add write block to cache
	while write_behind() is sending data */
	return _rfs_write(instance, path, buf, size, offset, desc);
}

int rfs_read(struct rfs_instance *instance, const char *path, char *buf, size_t size, off_t offset, uint64_t desc)
{
	DECORATE(_rfs_read, instance, path, buf, size, offset, desc);
}

int rfs_truncate(struct rfs_instance *instance, const char *path, off_t offset)
{
	DECORATE(_rfs_truncate, instance, path, offset);
}

int rfs_release(struct rfs_instance *instance, const char *path, uint64_t desc)
{
	DECORATE(_rfs_release, instance, path, desc);
}

int rfs_open(struct rfs_instance *instance, const char *path, int flags, uint64_t *desc)
{
	DECORATE(_rfs_open, instance, path, flags, desc);
}

int rfs_readdir(struct rfs_instance *instance, const char *path, const rfs_readdir_callback_t callback, void *callback_data)
{
	DECORATE(_rfs_readdir, instance, path, callback, callback_data);
}

int rfs_getattr(struct rfs_instance *instance, const char *path, struct stat *stbuf)
{
	DECORATE(_rfs_getattr, instance, path, stbuf);
}

int rfs_flush(struct rfs_instance *instance, const char *path, uint64_t desc)
{
	DECORATE(_rfs_flush, instance, path, desc);
}

int rfs_lock(struct rfs_instance *instance, const char *path, uint64_t desc, int cmd, struct flock *fl)
{
	DECORATE(_rfs_lock, instance, path, desc, cmd, fl);
}

int rfs_link(struct rfs_instance *instance, const char *path, const char *target)
{
	DECORATE(_rfs_link, instance, path, target);
}

int rfs_symlink(struct rfs_instance *instance, const char *path, const char *target)
{
	DECORATE(_rfs_symlink, instance, path, target);
}

int rfs_readlink(struct rfs_instance *instance, const char *path, char *buffer, size_t size)
{
	DECORATE(_rfs_readlink, instance, path, buffer, size);
}

#if defined ACL_OPERATIONS_AVAILABLE
int rfs_getxattr(struct rfs_instance *instance, const char *path, const char *name, char *value, size_t size)
{
	DECORATE(_rfs_getxattr, instance, path, name, value, size);
}

int rfs_setxattr(struct rfs_instance *instance, const char *path, const char *name, const char *value, size_t size, int flags)
{
	DECORATE(_rfs_setxattr, instance, path, name, value, size, flags);
}
#endif /* ACL_OPERATIONS_AVAILABLE */

int rfs_create(struct rfs_instance *instance, const char *path, mode_t mode, int flags, uint64_t *desc)
{
	DECORATE(_rfs_create, instance, path, mode, flags, desc);
}

#undef DECORATE

