
int rfs_mknod(const char *path, mode_t mode, dev_t dev)
{
	if (keep_alive_lock() == 0)
	{
		int ret = _rfs_mknod(path, mode, dev);
		keep_alive_unlock();
		return ret;
	}
	return -EIO;
}

int rfs_statfs(const char *path, struct statvfs *buf)
{
	if (keep_alive_lock() == 0)
	{
		int ret = _rfs_statfs(path, buf);
		keep_alive_unlock();
		return ret;
	}
	return -EIO;
}

int rfs_utime(const char *path, struct utimbuf *buf)
{
	if (keep_alive_lock() == 0)
	{
		int ret = _rfs_utime(path, buf);
		keep_alive_unlock();
		return ret;
	}
	return -EIO;	
}

int rfs_rename(const char *path, const char *new_path)
{
	if (keep_alive_lock() == 0)
	{
		int ret = _rfs_rename(path, new_path);
		keep_alive_unlock();
		return ret;
	}
	return -EIO;
}

int rfs_rmdir(const char *path)
{
	if (keep_alive_lock() == 0)
	{
		int ret = _rfs_rmdir(path);
		keep_alive_unlock();
		return ret;
	}
	return -EIO;
}

int rfs_unlink(const char *path)
{
	if (keep_alive_lock() == 0)
	{
		int ret = _rfs_unlink(path);
		keep_alive_unlock();
		return ret;
	}
	return -EIO;
}

int rfs_mkdir(const char *path, mode_t mode)
{
	if (keep_alive_lock() == 0)
	{
		int ret = _rfs_mkdir(path, mode);
		keep_alive_unlock();
		return ret;
	}
	return -EIO;
}

int rfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	if (keep_alive_lock() == 0)
	{
		int ret = _rfs_write(path, buf, size, offset, fi);
		keep_alive_unlock();
		return ret;
	}
	return -EIO;
}

int rfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	if (keep_alive_lock() == 0)
	{
		int ret = _rfs_read(path, buf, size, offset, fi);
		keep_alive_unlock();
		return ret;
	}
	return -EIO;
}

int rfs_truncate(const char *path, off_t offset)
{
	if (keep_alive_lock() == 0)
	{
		int ret = _rfs_truncate(path, offset);
		keep_alive_unlock();
		return ret;
	}
	return -EIO;
}

int rfs_release(const char *path, struct fuse_file_info *fi)
{
	if (keep_alive_lock() == 0)
	{
		int ret = _rfs_release(path, fi);
		keep_alive_unlock();
		return ret;
	}
	return -EIO;
}

int rfs_open(const char *path, struct fuse_file_info *fi)
{
	if (keep_alive_lock() == 0)
	{
		int ret = _rfs_open(path, fi);
		keep_alive_unlock();
		return ret;
	}
	return -EIO;
}

int rfs_readdir(const char *path, void *buf, const fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
	if (keep_alive_lock() == 0)
	{
		int ret = _rfs_readdir(path, buf, filler, offset, fi);
		keep_alive_unlock();
		return ret;
	}
	return -EIO;
}

int rfs_getattr(const char *path, struct stat *stbuf)
{
	if (keep_alive_lock() == 0)
	{
		int ret = _rfs_getattr(path, stbuf);
		keep_alive_unlock();
		return ret;
	}
	return -EIO;
}

int rfs_mount(const char *path)
{
	if (keep_alive_lock() == 0)
	{
		int ret = _rfs_mount(path);
		keep_alive_unlock();
		return ret;
	}
	return -EIO;
}

int rfs_request_salt()
{
	if (keep_alive_lock() == 0)
	{
		int ret = _rfs_request_salt();
		keep_alive_unlock();
		return ret;
	}
	return -EIO;
}

int rfs_auth(const char *user, const char *passwd)
{
	if (keep_alive_lock() == 0)
	{
		int ret = _rfs_auth(user, passwd);
		keep_alive_unlock();
		return ret;
	}
	return -EIO;
}

int rfs_flush(const char *path, struct fuse_file_info *fi)
{
	if (keep_alive_lock() == 0)
	{
		int ret = _rfs_flush(path, fi);
		keep_alive_unlock();
		return ret;
	}
	return -EIO;
}
