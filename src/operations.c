#include "operations.h"

#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>
#include <dirent.h>
#include <utime.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include "config.h"
#include "buffer.h"
#include "command.h"
#include "sendrecv.h"
#include "attr_cache.h"
#include "inet.h"
#include "keep_alive_client.h"
#include "write_cache.h"
#include "list.h"
#include "read_cache.h"

extern int g_server_socket;
static const unsigned cache_ttl = 60 * 5; // seconds
static pthread_t keep_alive_thread = 0;
extern struct rfs_config rfs_config;

struct fuse_operations rfs_operations = {
	.init		= rfs_init,
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
	.flush		= rfs_flush,
	
	.statfs		= rfs_statfs,
	
	// dummies
	.chmod		= rfs_chmod,
	.chown		= rfs_chown,
};

int _rfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int _rfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int _rfs_flush(const char *path, struct fuse_file_info *fi);

void* keep_alive()
{
	unsigned slept = 0;
	unsigned shorter_sleep = 1; // secs
	
	while (g_server_socket != -1)
	{
		sleep(1);
		slept += shorter_sleep;
		
		if (g_server_socket == -1)
		{
			pthread_exit(NULL);
		}
		
		if (slept >= keep_alive_period()
		&& keep_alive_trylock() == 0)
		{
			if (rfs_keep_alive() != 0)
			{
				keep_alive_unlock();
				pthread_exit(NULL);
			}
			
			keep_alive_unlock();
			slept = 0;
		}
	}
	
	return NULL;
}

void* rfs_init()
{
	keep_alive_init();
	pthread_create(&keep_alive_thread, NULL, keep_alive, NULL);
	
	return NULL;
}

void rfs_destroy(void *rfs_init_result)
{
	keep_alive_lock();
	rfs_disconnect(g_server_socket);
	g_server_socket = -1;
	keep_alive_unlock();
	
	pthread_join(keep_alive_thread, NULL);
	keep_alive_destroy();
	
	destroy_cache();
}

int _rfs_auth(const char *user, const char *passwd)
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

int _rfs_mount(const char *path)
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

int _rfs_getattr(const char *path, struct stat *stbuf)
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

int _rfs_readdir(const char *path, void *buf, const fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
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

int _rfs_open(const char *path, struct fuse_file_info *fi)
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

int _rfs_release(const char *path, struct fuse_file_info *fi)
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

int _rfs_truncate(const char *path, off_t offset)
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

int _rfs_read_cached(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	size_t size_to_read = size;

	if (read_cache_have_data(fi->fh, offset) >= size)
	{
		const char *cached_data = read_cache_get_data(fi->fh, size, offset);
		if (cached_data == NULL)
		{
			return -EIO;
		}
		
		memcpy(buf, cached_data, size);
		return size;
	}
	
	size_t cached_read_size = read_cache_size(fi->fh);
	if (cached_read_size > 0 
	&& size_to_read < read_cache_max_size())
	{
		if (cached_read_size * 2 <= read_cache_max_size())
		{
			size_to_read = cached_read_size * 2;
		}
		else
		{
			size_to_read = read_cache_max_size();
		}
	}
		
	destroy_read_cache();
	
	unsigned old_val = rfs_config.use_read_cache;
	rfs_config.use_read_cache = 0;
	
	char *buffer = get_buffer(size_to_read);
	
	int ret = _rfs_read(path, buffer, size_to_read, offset, fi);
	if (ret < 0)
	{
		rfs_config.use_read_cache = old_val;
		free_buffer(buffer);
		return ret;
	}
	
	rfs_config.use_read_cache = old_val;
	
	memcpy(buf, buffer, ret < size ? ret : size);
	if (ret > size)
	{
		put_to_read_cache(fi->fh, buffer, size_to_read, offset);
	}
	
	return ret == size_to_read ? size : ret;
}

int _rfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	if (g_server_socket == -1)
	{
		return -EIO;
	}
	
	if (rfs_config.use_read_cache)
	{
		return _rfs_read_cached(path, buf, size, offset, fi);
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

int _rfs_flush(const char *path, struct fuse_file_info *fi)
{
	if (rfs_config.use_read_cache != 0)
	{
		destroy_read_cache();
	}

	if (rfs_config.use_write_cache == 0)
	{
		return 0;
	}
	
	if (get_write_cache_size() < 1)
	{
		return 0;
	}
	
	if (get_write_cache() == NULL)
	{
		return -EIO;
	}
	
	const struct list *write_cache = get_write_cache();
	const struct write_cache_entry *first_entry = (const struct write_cache_entry *)write_cache->data;
	
	uint64_t handle = fi->fh;
	uint32_t fsize = get_write_cache_size();
	uint32_t foffset = first_entry->offset;
	
	size_t overall_size = sizeof(handle) + sizeof(fsize) + sizeof(foffset) + fsize;
	
	char *buffer = get_buffer(fsize);

	size_t done = 0;
	while (write_cache)
	{
		if (done > overall_size)
		{
			free_buffer(buffer);
			return -EIO;
		}
		
		struct write_cache_entry *item = (struct write_cache_entry *)write_cache->data;
		
		memcpy(buffer + done, item->buffer, item->size);
		done += item->size;
		
		write_cache = write_cache->next;
	}
	
	destroy_write_cache();
	
	unsigned old_val = rfs_config.use_write_cache;
	rfs_config.use_write_cache = 0;
	
	int ret = _rfs_write(path, buffer, fsize, foffset, fi);
	
	DEBUG("write ret: %d\n", ret);
	
	rfs_config.use_write_cache = old_val;
	return ret == fsize ? 0 : ret;
}

int _rfs_write_cached(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	if (is_fit_to_write_cache(fi->fh, size, offset))
	{
		return (add_to_write_cache(fi->fh, buf, size, offset) != 0 ? -EIO : size);
	}
	else
	{
		int ret = _rfs_flush(path, fi);
		if (ret < 0)
		{
			return ret;
		}
		return (add_to_write_cache(fi->fh, buf, size, offset) != 0 ? -EIO : size);
	}
}

int _rfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	if (g_server_socket == -1)
	{
		return -EIO;
	}
	
	if (rfs_config.use_write_cache != 0)
	{
		return _rfs_write_cached(path, buf, size, offset, fi);
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

int _rfs_mkdir(const char *path, mode_t mode)
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

int _rfs_unlink(const char *path)
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

int _rfs_rmdir(const char *path)
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

int _rfs_rename(const char *path, const char *new_path)
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

int _rfs_utime(const char *path, struct utimbuf *buf)
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

int _rfs_statfs(const char *path, struct statvfs *buf)
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

int _rfs_mknod(const char *path, mode_t mode, dev_t dev)
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

int rfs_keep_alive()
{
	if (g_server_socket == -1)
	{
		return -EIO;
	}
	
	struct command cmd = { cmd_keepalive, 0 };
	
	if (rfs_send_cmd(g_server_socket, &cmd) == -1)
	{
		return -EIO;
	}
	
	return 0;
}

#include "operations_sync.c"
