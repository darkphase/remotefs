#include "operations.h"

#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>
#include <dirent.h>
#include <utime.h>
#include <sys/statvfs.h>
#include <unistd.h>

#include "config.h"
#include "buffer.h"
#include "command.h"
#include "sendrecv.h"
#include "attr_cache.h"
#include "inet.h"

extern int g_server_socket;
static const unsigned cache_ttl = 60 * 5; // seconds

struct fuse_operations rfs_operations = {
	.destroy	= rfs_destroy,
	
    .getattr	= rfs_getattr,
	
	.readdir	= rfs_readdir,
	.mkdir		= rfs_mkdir,
	.unlink		= rfs_unlink,
	.rmdir		= rfs_rmdir,
	.rename		= rfs_rename,
	.utime		= rfs_utime,
	
	.mknod		= rfs_mknod, // regular files only
	.open 		= rfs_open,
	.release	= rfs_release,
	.read 		= rfs_read,
	.write		= rfs_write,
	.truncate	= rfs_truncate,
	
	.statfs		= rfs_statfs,
	
	// dummies
	.chmod		= rfs_chmod,
	.chown		= rfs_chown,
};

void rfs_destroy(void *rfs_init_result)
{
	rfs_disconnect(g_server_socket);
	
	destroy_cache();
}

int rfs_auth(const char *user, const char *passwd)
{
	if (g_server_socket == -1)
	{
		return -EIO;
	}

	uint32_t passwd_len = strlen(passwd) + 1;
	unsigned user_len = strlen(user) + 1;
	unsigned overall_size = sizeof(passwd_len) + passwd_len + user_len;
	
	struct command cmd = { cmd_auth, overall_size };
	
	if (rfs_send_cmd(g_server_socket, &cmd) == -1)
	{
		return -EIO;
	}
	
	char *buffer = get_buffer(overall_size);
	
	pack(user, user_len, buffer, 
	pack(passwd, passwd_len, buffer, 
	pack_32(&passwd_len, buffer, 0
		)));
	
	if (rfs_send_data(g_server_socket, buffer, cmd.data_len) == -1)
	{
		free_buffer(buffer);
		return -EIO;
	}
	
	free_buffer(buffer);
	
	return 0;
}

int rfs_mount(const char *path)
{
	if (g_server_socket == -1)
	{
		return -EIO;
	}

	unsigned path_len = strlen(path);
	struct command cmd = { cmd_changepath, path_len + 1};

	if (rfs_send_cmd(g_server_socket, &cmd) == -1)
	{
		return -EIO;
	}

	if (rfs_send_data(g_server_socket, path, cmd.data_len) == -1)
	{
		return -EIO;
	}

	struct answer ans = { 0 };

	if (rfs_receive_answer(g_server_socket, &ans) == -1)
	{
		return -EIO;
	}

	if (ans.command != cmd_changepath)
	{
		return -EIO;
	}

	return ans.ret == -1 ? -ans.ret_errno : ans.ret;
}

int rfs_getattr(const char *path, struct stat *stbuf)
{
	if (g_server_socket == -1)
	{
		return -EIO;
	}
	
	if (cache_is_old() != 0)
	{
		clear_cache();
	}
	
	struct tree_item *cached_value = get_cache(path);
	if (cached_value != NULL 
	&& (time(NULL) - cached_value->time) < cache_ttl )
	{
		DEBUG("%s is cached\n", path);
		memcpy(stbuf, &cached_value->data, sizeof(*stbuf));
		return 0;
	}
	
	unsigned path_len = strlen(path) + 1;

	struct command cmd = { cmd_getattr, path_len };
	if (rfs_send_cmd(g_server_socket, &cmd) == -1)
	{
		return -EIO;
	}
	
	if (rfs_send_data(g_server_socket, path, path_len) == -1)
	{
		return -EIO;
	}
	
	struct answer ans = { 0 };
	if (rfs_receive_answer(g_server_socket, &ans) == -1)
	{
		return -EIO;	}
	
	if (ans.command != cmd_getattr)
	{
		return -EIO;	}
	
	if (ans.ret == -1)
	{
		return -ans.ret_errno;
	}
	
	char *buffer = get_buffer(ans.data_len);
	
	if (rfs_receive_data(g_server_socket, buffer, ans.data_len) == -1)
	{
		free_buffer(buffer);
		return -EIO;
	}
	
	uint32_t mode = 0;
	uint32_t uid = 0;
	uint32_t gid = 0;
	uint32_t size = 0;
	uint32_t atime = 0;
	uint32_t mtime = 0;
	uint32_t ctime = 0;
	
	unpack_32(&ctime, buffer, 
	unpack_32(&mtime, buffer, 
	unpack_32(&atime, buffer, 
	unpack_32(&size, buffer, 
	unpack_32(&gid, buffer, 
	unpack_32(&uid, buffer, 
	unpack_32(&mode, buffer, 0 
		)))))));
		
	free_buffer(buffer);
	
	struct stat result = { 0 };
		
	result.st_mode = mode;
	result.st_uid = getuid();
	result.st_gid = getgid();
	result.st_size = size;
	result.st_atime = atime;
	result.st_mtime = mtime;
	result.st_ctime = ctime;
	
	memcpy(stbuf, &result, sizeof(*stbuf));
	
//	dump(&result, sizeof(result));
	
	if (cache_file(path, &result) == NULL)
	{
		return -EIO;
	}
	
	return ans.ret == -1 ? -ans.ret_errno : ans.ret;}

int rfs_readdir(const char *path, void *buf, const fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
	if (g_server_socket == -1)
	{
		return -EIO;
	}
	
	unsigned path_len = strlen(path) + 1;

	struct command cmd = { cmd_readdir, path_len };
	
	if (rfs_send_cmd(g_server_socket, &cmd) == -1)
	{
		return -EIO;
	}
	
	if (rfs_send_data(g_server_socket, (void *)path, path_len) == -1)
	{
		return -EIO;
	}
	
	struct answer ans = { 0 };
	char *buffer = 0;
	uint32_t type = 0;
	
	do
	{
		if (rfs_receive_answer(g_server_socket, &ans) == -1)
		{
			return -EIO;
		}
		
		if (ans.command != cmd_readdir)
		{
			return -EIO;
		}
		
		if (ans.ret == -1)
		{
			return -ans.ret_errno;
		}
		
		if (ans.data_len == 0)
		{
			break;
		}
		
		if (buffer == NULL)
		{
			buffer = get_buffer(ans.data_len);
		}
		
		if (rfs_receive_data(g_server_socket, buffer, ans.data_len) == -1)
		{
			if (buffer != NULL)
			{
				free_buffer(buffer);
			}
	
			return -EIO;
		}
		
		char *name = buffer 
		+ unpack_32(&type, buffer, 0);
		
		if (filler(buf, name, NULL, 0))
		{
			break;
		}
	}
	while (ans.data_len > 0);
	
	if (buffer != 0)
	{
		free_buffer(buffer);
	}
	
	return 0;}

int rfs_open(const char *path, struct fuse_file_info *fi)
{
	if (g_server_socket == -1)
	{
		return -EIO;
	}
	
	unsigned path_len = strlen(path) + 1;
	uint16_t fi_flags = 0;
	if (fi->flags & O_APPEND) { fi_flags |= RFS_APPEND; }
	if (fi->flags & O_ASYNC) { fi_flags |= RFS_ASYNC; }
	if (fi->flags & O_CREAT) { fi_flags |= RFS_CREAT; }
	if (fi->flags & O_EXCL) { fi_flags |= RFS_EXCL; }
	if (fi->flags & O_NONBLOCK) { fi_flags |= RFS_NONBLOCK; }
	if (fi->flags & O_NDELAY) { fi_flags |= RFS_NDELAY; }
	if (fi->flags & O_SYNC) { fi_flags |= RFS_SYNC; }
	if (fi->flags & O_TRUNC) { fi_flags |= RFS_TRUNC; }
	if (fi->flags & O_RDONLY) { fi_flags |= RFS_RDONLY; }
	if (fi->flags & O_WRONLY) { fi_flags |= RFS_WRONLY; }
	if (fi->flags & O_RDWR) { fi_flags |= RFS_RDWR; }
	
	unsigned overall_size = sizeof(fi_flags) + path_len;
	
	struct command cmd = { cmd_open, overall_size };
	
	if (rfs_send_cmd(g_server_socket, &cmd) == -1)
	{
		return -EIO;
	}
	
	char *buffer = get_buffer(cmd.data_len);
	
	pack(path, path_len, buffer, 
	pack_16(&fi_flags, buffer, 0
		));
	
	if (rfs_send_data(g_server_socket, buffer, cmd.data_len) == -1)
	{
		free_buffer(buffer);
		return -EIO;
	}
	
	free_buffer(buffer);
	
	struct answer ans = { 0 };
	
	if (rfs_receive_answer(g_server_socket, &ans) == -1)
	{
		return -EIO;
	}
	
	if (ans.command != cmd_open)
	{
		return -EIO;
	}
	
	if (ans.ret != -1)
	{
		uint64_t handle = (uint64_t)-1;	
		
		if (ans.data_len != sizeof(handle))
		{
			return -EIO;
		}
		
		if (rfs_receive_data(g_server_socket, &handle, ans.data_len) == -1)
		{
			return -EIO;
		}
		
		fi->fh = ntohll(handle);
	}
	
	if (ans.ret != -1)
	{
		delete_from_cache(path);
	}
	
	return ans.ret == -1 ? -ans.ret_errno : ans.ret;
}

int rfs_release(const char *path, struct fuse_file_info *fi)
{
	if (g_server_socket == -1)
	{
		return -EIO;
	}

	uint64_t handle = htonll(fi->fh);

	struct command cmd = { cmd_release, sizeof(handle) };
	
	if (rfs_send_cmd(g_server_socket, &cmd) == -1)
	{
		return -EIO;
	}
	
	if (rfs_send_data(g_server_socket, &handle, cmd.data_len) == -1)
	{
		return -EIO;
	}
	
	struct answer ans = { 0 };
	
	if (rfs_receive_answer(g_server_socket, &ans) == 0)
	{
		return -EIO;
	}
	
	if (ans.command != cmd_release || ans.data_len != 0)
	{
		return -EIO;
	}
	
	return ans.ret == -1 ? -ans.ret_errno : ans.ret;
}

int rfs_truncate(const char *path, off_t offset)
{
	if (g_server_socket == -1)
	{
		return -EIO;
	}
	
	unsigned path_len = strlen(path) + 1;
	uint32_t foffset = offset;
	
	unsigned overall_size = sizeof(foffset) + path_len;
	
	struct command cmd = { cmd_truncate, overall_size };
	
	if (rfs_send_cmd(g_server_socket, &cmd) == -1)
	{
		return -EIO;
	}
	
	char *buffer = get_buffer(cmd.data_len);
	
	pack(path, path_len, buffer, 
	pack_32(&foffset, buffer, 0
		));
	
	if (rfs_send_data(g_server_socket, buffer, cmd.data_len) == -1)
	{
		free_buffer(buffer);
		return -EIO;
	}
	
	free_buffer(buffer);
	
	struct answer ans = { 0 };
	
	if (rfs_receive_answer(g_server_socket, &ans) == -1)
	{
		return -EIO;
	}
	
	if (ans.command != cmd_truncate || ans.data_len != 0)
	{
		return -EIO;
	}
	
	return ans.ret == -1 ? -ans.ret_errno : ans.ret;
}

int rfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	if (g_server_socket == -1)
	{
		return -EIO;
	}
	
	uint64_t handle = fi->fh;
	uint32_t fsize = size;
	uint32_t foffset = offset;
	
	unsigned overall_size = sizeof(fsize) + sizeof(foffset) + sizeof(handle);

	struct command cmd = { cmd_read, overall_size };
	
	if (rfs_send_cmd(g_server_socket, &cmd) == -1)
	{
		return -EIO;
	}
	
	char *buffer = get_buffer(cmd.data_len);
	
	pack_64(&handle, buffer, 
	pack_32(&foffset, buffer, 
	pack_32(&fsize, buffer, 0
		)));
	
	if (rfs_send_data(g_server_socket, buffer, cmd.data_len) == -1)
	{
		free_buffer(buffer);
		return -EALREADY;
	}
	
	free_buffer(buffer);
	
	struct answer ans = { 0 };
	
	if (rfs_receive_answer(g_server_socket, &ans) == -1)
	{
		return -EIO;
	}
	
	if (ans.command != cmd_read)
	{
		return -EIO;
	}
	
	if (ans.ret == -1 || ans.data_len > size)
	{
		return -ans.ret_errno;
	}
	
	if (ans.data_len > 0)
	{
		if (rfs_receive_data(g_server_socket, buf, ans.data_len) == -1)
		{
			return -EIO;
		}
	}
	
	return ans.data_len;
}

int rfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	if (g_server_socket == -1)
	{
		return -EIO;
	}
	
	uint64_t handle = fi->fh;
	uint32_t fsize = size;
	uint32_t foffset = offset;
	
	unsigned overall_size = size + sizeof(fsize) + sizeof(foffset) + sizeof(handle);

	struct command cmd = { cmd_write, overall_size };
	
	if (rfs_send_cmd(g_server_socket, &cmd) == -1)
	{
		return -EIO;
	}
	
	char *buffer = get_buffer(cmd.data_len);
	
	pack(buf, size, buffer, 
	pack_64(&handle, buffer, 
	pack_32(&foffset, buffer, 
	pack_32(&fsize, buffer, 0
		))));
	
	if (rfs_send_data(g_server_socket, buffer, cmd.data_len) == -1)
	{
		free_buffer(buffer);
		return -EIO;
	}
	
	free_buffer(buffer);
	
	struct answer ans = { 0 };
	
	if (rfs_receive_answer(g_server_socket, &ans) == -1)
	{
		return -EIO;
	}
	
	if (ans.command != cmd_write || ans.data_len != 0)
	{
		return -EIO;
	}
	
	return ans.ret == -1 ? -ans.ret_errno : (int)ans.ret;
}

int rfs_mkdir(const char *path, mode_t mode)
{
	if (g_server_socket == -1)
	{
		return -EIO;
	}
	
	unsigned path_len = strlen(path) + 1;
	
	uint32_t fmode = mode;
	
	unsigned overall_size = sizeof(fmode) + path_len;

	struct command cmd = { cmd_mkdir, overall_size };
	
	if (rfs_send_cmd(g_server_socket, &cmd) == -1)
	{
		return -EIO;
	}
	
	char *buffer = get_buffer(cmd.data_len);
	
	pack(path, path_len, buffer, 
	pack_32(&fmode, buffer, 0
		));
	
	if (rfs_send_data(g_server_socket, buffer, cmd.data_len) == -1)
	{
		free_buffer(buffer);
		return -EIO;
	}
	
	free_buffer(buffer);
	
	struct answer ans = { 0 };
	
	if (rfs_receive_answer(g_server_socket, &ans) == -1)
	{
		return -EIO;
	}
	
	if (ans.command != cmd_mkdir || ans.data_len != 0)
	{
		return -EIO;
	}
	
	return ans.ret == -1 ? -ans.ret_errno : ans.ret;
}

int rfs_unlink(const char *path)
{
	if (g_server_socket == -1)
	{
		return -EIO;
	}
	
	unsigned path_len = strlen(path) + 1;
	struct command cmd = { cmd_unlink, path_len };
	
	if (rfs_send_cmd(g_server_socket, &cmd) == -1)
	{
		return -EIO;
	}
	
	if (rfs_send_data(g_server_socket, path, cmd.data_len) == -1)
	{
		return -EIO;
	}
	
	struct answer ans = { 0 };
	
	if (rfs_receive_answer(g_server_socket, &ans) == -1)
	{
		return -EIO;
	}
	
	if (ans.command != cmd_unlink || ans.data_len != 0)
	{
		return -EIO;
	}
	
	if (ans.ret == 0)
	{
		delete_from_cache(path);
	}
	
	return ans.ret == -1 ? -ans.ret_errno : ans.ret;
}

int rfs_rmdir(const char *path)
{
	if (g_server_socket == -1)
	{
		return -EIO;
	}
	
	unsigned path_len = strlen(path) + 1;
	struct command cmd = { cmd_rmdir, path_len };
	
	if (rfs_send_cmd(g_server_socket, &cmd) == -1)
	{
		return -EIO;
	}
	
	if (rfs_send_data(g_server_socket, path, cmd.data_len) == -1)
	{
		return -EIO;
	}
	
	struct answer ans = { 0 };
	
	if (rfs_receive_answer(g_server_socket, &ans) == -1)
	{
		return -EIO;
	}
	
	if (ans.command != cmd_rmdir || ans.data_len != 0)
	{
		return -EIO;
	}
	
	if (ans.ret == 0)
	{
		delete_from_cache(path);
	}
	
	return ans.ret == -1 ? -ans.ret_errno : ans.ret;
}

int rfs_rename(const char *path, const char *new_path)
{
	if (g_server_socket == -1)
	{
		return -EIO;
	}
	
	unsigned path_len = strlen(path) + 1;
	unsigned new_path_len = strlen(new_path) + 1;
	uint32_t len = path_len;
	
	unsigned overall_size = sizeof(len) + path_len + new_path_len;
	
	struct command cmd = { cmd_rename, overall_size };
	
	if (rfs_send_cmd(g_server_socket, &cmd) == -1)
	{
		return -EIO;
	}
	
	char *buffer = get_buffer(cmd.data_len);
	
	pack(new_path, new_path_len, buffer,
	pack(path, path_len, buffer,
	pack_32(&len, buffer, 0
		)));
	
	if (rfs_send_data(g_server_socket, buffer, cmd.data_len) == -1)
	{
		free_buffer(buffer);
		return -EIO;
	}
	
	free_buffer(buffer);
	
	struct answer ans = { 0 };
	
	if (rfs_receive_answer(g_server_socket, &ans) == -1)
	{
		return -EIO;
	}
	
	if (ans.command != cmd_rename || ans.data_len != 0)
	{
		return -EIO;
	}
	
	if (ans.ret == 0)
	{
		delete_from_cache(path);
	}
	
	return ans.ret == -1 ? -ans.ret_errno : ans.ret;
}

int rfs_utime(const char *path, struct utimbuf *buf)
{
	if (g_server_socket == -1)
	{
		return -EIO;
	}
	
	unsigned path_len = strlen(path) + 1;
	uint32_t actime = 0;
	uint32_t modtime = 0;
	uint16_t is_null = (buf == 0 ? 1 : 0);
	
	if (buf != 0)
	{
		actime = buf->actime;
		modtime = buf->modtime;
	}
	
	unsigned overall_size = path_len + sizeof(actime) + sizeof(modtime) + sizeof(is_null);
	
	struct command cmd = { cmd_utime, overall_size };
	
	if (rfs_send_cmd(g_server_socket, &cmd) == -1)
	{
		return -EIO;
	}
	
	char *buffer = get_buffer(cmd.data_len);
	
	pack(path, path_len, buffer, 
	pack_32(&actime, buffer, 
	pack_32(&modtime, buffer, 
	pack_16(&is_null, buffer, 0
		))));
	
	if (rfs_send_data(g_server_socket, buffer, cmd.data_len) == -1)
	{
		free_buffer(buffer);
		return -EIO;
	}
	
	free_buffer(buffer);
	
	struct answer ans = { 0 };
	
	if (rfs_receive_answer(g_server_socket, &ans) == -1)
	{
		return -EIO;
	}
	
	if (ans.command != cmd_utime || ans.data_len != 0)
	{
		return -EIO;
	}
	
	return ans.ret == -1 ? -ans.ret_errno : ans.ret;
}

int rfs_statfs(const char *path, struct statvfs *buf)
{
	if (g_server_socket == -1)
	{
		return -EIO;
	}
	
	unsigned path_len = strlen(path) + 1;
	
	struct command cmd = { cmd_statfs, path_len };
	
	if (rfs_send_cmd(g_server_socket, &cmd) == -1)
	{
		return -EIO;	}

	if (rfs_send_data(g_server_socket, path, path_len) == -1)
	{
		return -EIO;
	}
	
	struct answer ans = { 0 };
	
	if (rfs_receive_answer(g_server_socket, &ans) == -1)
	{
		return -EIO;
	}
	
	if (ans.ret == -1)
	{
		return -ans.ret_errno;
	}
	
	uint32_t bsize = 0;
	uint32_t blocks = 0;
	uint32_t bfree = 0;
	uint32_t bavail = 0;
	uint32_t files = 0;
	uint32_t ffree = 0;
	uint32_t namemax = 0;
	
	unsigned overall_size = sizeof(bsize)
	+ sizeof(blocks)
	+ sizeof(bfree)
	+ sizeof(bavail)
	+ sizeof(files)
	+ sizeof(ffree)
	+ sizeof(namemax);
	
	if (ans.command != cmd_statfs || ans.data_len != overall_size)
	{
		return -EIO;
	}
	
	char *buffer = get_buffer(ans.data_len);
	
	if (rfs_receive_data(g_server_socket, buffer, ans.data_len) == -1)
	{
		free_buffer(buffer);
		return -EIO;
	}
	
	unpack_32(&namemax, buffer, 
	unpack_32(&ffree, buffer, 
	unpack_32(&files, buffer, 
	unpack_32(&bavail, buffer, 
	unpack_32(&bfree, buffer, 
	unpack_32(&blocks, buffer, 
	unpack_32(&bsize, buffer, 0
		)))))));
		
	free_buffer(buffer);
	
	buf->f_bsize = bsize;
	buf->f_blocks = blocks;
	buf->f_bfree = bfree;
	buf->f_bavail = bavail;
	buf->f_files = files;
	buf->f_ffree = ffree;
	buf->f_namemax = namemax;
	
	return ans.ret;
}

int rfs_mknod(const char *path, mode_t mode, dev_t dev)
{
	if (g_server_socket == -1)
	{
		return -EIO;
	}
	
	unsigned path_len = strlen(path) + 1;
	uint32_t fmode = mode;
	
	unsigned overall_size = sizeof(fmode) + path_len;
	
	struct command cmd = { cmd_mknod, overall_size };
	
	if (rfs_send_cmd(g_server_socket, &cmd) == -1)
	{
		return -EIO;
	}
	
	char *buffer = get_buffer(overall_size);

	pack(path, path_len, buffer, 
	pack_32(&fmode, buffer, 0
		));

	if (rfs_send_data(g_server_socket, buffer, cmd.data_len) == -1)
	{
		free_buffer(buffer);
		return -EIO;
	}
	
	free_buffer(buffer);
	
	struct answer ans = { 0 };
	
	if (rfs_receive_answer(g_server_socket, &ans) == -1)
	{
		return -EIO;
	}
	
	if (ans.command != cmd_mknod)
	{
		return -EIO;
	}

	return ans.ret == 0 ? 0 : -ans.ret_errno;
}

int rfs_chmod(const char *path, mode_t mode)
{
	return 0;
}

int rfs_chown(const char *path, uid_t uid, gid_t gid)
{
	return 0;
}
