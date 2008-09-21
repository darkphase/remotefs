/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

/* syncronized operations. will lock keep alive when it's needed */

/* if connection lost after executing the operation
and operation failed, then try it one more time */
#define DECORATE(func, args...)                             \
	int ret = -EIO;                                     \
	if (keep_alive_lock() == 0)                         \
	{                                                   \
		if(check_connection() == 0)                 \
		{                                           \
			ret = func(args);                   \
			if (ret == -EIO                     \
			&& rfs_is_connection_lost() != 0    \
			&& check_connection() == 0)         \
			{                                   \
				ret = func(args);           \
			}                                   \
		}                                           \
		keep_alive_unlock();                        \
	}                                                   \
	return ret;

int rfs_mknod(const char *path, mode_t mode, dev_t dev)
{
	DECORATE(_rfs_mknod, path, mode, dev);
}

int rfs_chmod(const char *path, mode_t mode)
{
	DECORATE(_rfs_chmod, path, mode);
}

int rfs_chown(const char *path, uid_t uid, gid_t gid)
{
	DECORATE(_rfs_chown, path, uid, gid);
}

int rfs_statfs(const char *path, struct statvfs *buf)
{
	DECORATE(_rfs_statfs, path, buf);
}

int rfs_utime(const char *path, struct utimbuf *buf)
{
	DECORATE(_rfs_utime, path, buf);
}

int rfs_rename(const char *path, const char *new_path)
{
	DECORATE(_rfs_rename, path, new_path);
}

int rfs_rmdir(const char *path)
{
	DECORATE(_rfs_rmdir, path);
}

int rfs_unlink(const char *path)
{
	DECORATE(_rfs_unlink, path);
}

int rfs_mkdir(const char *path, mode_t mode)
{
	DECORATE(_rfs_mkdir, path, mode);
}

int rfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	DECORATE(_rfs_write, path, buf, size, offset, fi);
}

int rfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	DECORATE(_rfs_read, path, buf, size, offset, fi);
}

int rfs_truncate(const char *path, off_t offset)
{
	DECORATE(_rfs_truncate, path, offset);
}

int rfs_release(const char *path, struct fuse_file_info *fi)
{
	DECORATE(_rfs_release, path, fi);
}

int rfs_open(const char *path, struct fuse_file_info *fi)
{
	DECORATE(_rfs_open, path, fi);
}

int rfs_readdir(const char *path, void *buf, const fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
	DECORATE(_rfs_readdir, path, buf, filler, offset, fi);
}

int rfs_getattr(const char *path, struct stat *stbuf)
{
	DECORATE(_rfs_getattr, path, stbuf);
}

int rfs_flush(const char *path, struct fuse_file_info *fi)
{
	DECORATE(_rfs_flush, path, fi);
}

#undef DECORATE
