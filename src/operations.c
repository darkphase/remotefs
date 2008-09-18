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
#include <stdlib.h>
#ifdef FREEBSD
#	include <sys/socket.h>
#endif

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
#include "crypt.h"
#include "id_lookup.h"
#include "path.h"

extern int g_server_socket;
extern struct rfs_config rfs_config;

static const unsigned cache_ttl = 60 * 5; /* seconds */
static pthread_t keep_alive_thread = 0;
static char auth_salt[MAX_SALT_LEN + 1] = { 0 };
static enum rfs_export_opts export_opts = opt_none;
static uid_t my_uid = (uid_t)-1;
static uid_t my_gid = (gid_t)-1;

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
	.mknod		= rfs_mknod, /* regular files only */
	.open 		= rfs_open,
	.release	= rfs_release,
	.read 		= rfs_read,
	.write		= rfs_write,
	.truncate	= rfs_truncate,
	.flush		= rfs_flush,
	.statfs		= rfs_statfs,
	.chmod		= rfs_chmod,
	.chown		= rfs_chown,
};

int _rfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int _rfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int _rfs_flush(const char *path, struct fuse_file_info *fi);

int check_connection()
{
	if (rfs_is_connection_lost() == 0)
	{
		return 0;
	}
	
	destroy_uids_lookup();
	destroy_gids_lookup();
	
	if (rfs_reconnect(0) == 0)
	{
		rfs_set_connection_restored();
		return 0;
	}

	return -1;
}

void* maintenance(void *ignored)
{
	unsigned keep_alive_slept = 0;
	unsigned attr_cache_slept = 0;
	unsigned shorter_sleep = 1; /* secs */

	while (g_server_socket != -1)
	{
		sleep(shorter_sleep);
		keep_alive_slept += shorter_sleep;
		attr_cache_slept += shorter_sleep;
		
		if (g_server_socket == -1)
		{
			pthread_exit(NULL);
		}
		
		if (keep_alive_slept >= keep_alive_period()
		&& keep_alive_trylock() == 0)
		{
			if (check_connection() == 0)
			{
				if (rfs_keep_alive() != 0)
				{
					keep_alive_unlock();
					pthread_exit(NULL);
				}
			}
			
			keep_alive_unlock();
			keep_alive_slept = 0;
		}
		
		if (attr_cache_slept >= ATTR_CACHE_TTL
		&& keep_alive_lock() == 0)
		{
			if (cache_is_old() != 0)
			{
				clear_cache();
			}
			
			keep_alive_unlock();
			attr_cache_slept = 0;
		}
	}

	return NULL;
}

int rfs_reconnect(int show_errors)
{
	DEBUG("(re)connecting to %s:%d\n", rfs_config.host, rfs_config.server_port);
	
	int sock = rfs_connect(rfs_config.host, rfs_config.server_port);
	if (sock < 0)
	{
		if (show_errors != 0)
		{
			ERROR("Error connecting to remote host: %s\n", strerror(-sock));
		}
		return 1;
	}

	if (rfs_config.auth_user != NULL 
	&& rfs_config.auth_passwd != NULL)
	{
		DEBUG("authenticating as %s with pwd %s\n", rfs_config.auth_user, rfs_config.auth_passwd);
	
		int req_ret = rfs_request_salt();
		if (req_ret != 0)
		{
			if (show_errors != 0)
			{
				ERROR("Requesting salt for authentication error: %s\n", strerror(-req_ret));
			}
			rfs_disconnect(sock, 1);
			return -1;
		}
	
		int auth_ret = rfs_auth(rfs_config.auth_user, rfs_config.auth_passwd);
		if (auth_ret != 0)
		{
			if (show_errors != 0)
			{
				ERROR("Authentication error: %s\n", strerror(-auth_ret));
			}
			rfs_disconnect(sock, 1);
			return -1;
		}
	}

	DEBUG("mounting %s\n", rfs_config.path);

	int mount_ret = rfs_mount(rfs_config.path);
	if (mount_ret != 0)
	{
		if (show_errors != 0)
		{
			ERROR("Error mounting remote directory: %s\n", strerror(-mount_ret));
		}
		rfs_disconnect(sock, 1);
		return -1;
	}
	
	int getopts_ret = rfs_getexportopts(&export_opts);
	if (getopts_ret != 0)
	{
		if (show_errors != 0)
		{
			ERROR("Error getting export options from server: %s\n", strerror(-getopts_ret));
		}
		rfs_disconnect(sock, 1);
		return -1;
	}
	
	if ((export_opts & opt_ugo) > 0)
	{
		create_uids_lookup();
		create_gids_lookup();
	}

	DEBUG("%s\n", "all ok");
	return 0;
}

void* rfs_init()
{
	my_uid = getuid();
	my_gid = getgid();
	
	keep_alive_init();
	pthread_create(&keep_alive_thread, NULL, maintenance, NULL);

	return NULL;
}

void rfs_destroy(void *rfs_init_result)
{
	keep_alive_lock();
	rfs_disconnect(g_server_socket, 1);
	g_server_socket = -1;
	keep_alive_unlock();

	pthread_join(keep_alive_thread, NULL);
	keep_alive_destroy();
	
	destroy_cache();
	
//	destroy_uids_lookup();
//	destroy_gids_lookup();
}

void rfs_disconnect(int sock, int gently)
{
	if (gently != 0)
	{
		struct command cmd = { cmd_closeconnection, 0 };
		rfs_send_cmd(sock, &cmd);
	}
	
	close(sock);
	shutdown(sock, SHUT_RDWR);
}

int rfs_request_salt()
{
	if (g_server_socket == -1)
	{
		return -EIO;
	}

	memset(auth_salt, 0, sizeof(auth_salt));

	struct command cmd = { cmd_request_salt, 0 };

	if (rfs_send_cmd(g_server_socket, &cmd) == -1)
	{
		return -EIO;
	}

	struct answer ans = { 0 };

	if (rfs_receive_answer(g_server_socket, &ans) == -1)
	{
		return -EIO;
	}

	if (ans.command != cmd_request_salt
	|| (ans.ret == 0 && (ans.data_len < 1 || ans.data_len > sizeof(auth_salt))))
	{
		return -EIO;
	}

	if (ans.ret == 0)
	{
		if (rfs_receive_data(g_server_socket, auth_salt, ans.data_len) == -1)
		{
			return -EIO;
		}
	}

	return -ans.ret;
}

int rfs_auth(const char *user, const char *passwd)
{
	if (g_server_socket == -1)
	{
		return -EIO;
	}

	char *crypted = passwd_hash(passwd, auth_salt);

	memset(auth_salt, 0, sizeof(auth_salt));

	uint32_t crypted_len = strlen(crypted) + 1;
	unsigned user_len = strlen(user) + 1;
	unsigned overall_size = sizeof(crypted_len) + crypted_len + user_len;

	struct command cmd = { cmd_auth, overall_size };


	char *buffer = get_buffer(overall_size);

	pack(user, user_len, buffer, 
	pack(crypted, crypted_len, buffer, 
	pack_32(&crypted_len, buffer, 0
		)));

	if (rfs_send_cmd_data(g_server_socket, &cmd, buffer, cmd.data_len) == -1)
	{
		free_buffer(buffer);
		free(crypted);
		return -EIO;
	}

	free_buffer(buffer);
	free(crypted);

	struct answer ans = { 0 };

	if (rfs_receive_answer(g_server_socket, &ans) == -1)
	{
		return -EIO;
	}

	if (ans.command != cmd_auth
	|| ans.data_len > 0)
	{
		return -EIO;
	}

	return -ans.ret;
}

int rfs_mount(const char *path)
{
	if (g_server_socket == -1)
	{
		return -EIO;
	}

	unsigned path_len = strlen(path);
	struct command cmd = { cmd_changepath, path_len + 1};
	if (rfs_send_cmd_data(g_server_socket, &cmd, path, cmd.data_len) == -1)
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

int rfs_getexportopts(enum rfs_export_opts *opts)
{
	if (g_server_socket == -1)
	{
		return -EIO;
	}

	*opts = opt_none;
	
	struct command cmd = { cmd_getexportopts, 0 };
	
	if (rfs_send_cmd(g_server_socket, &cmd) == -1)
	{
		return -EIO;
	}
	
	struct answer ans = { 0 };
	
	if (rfs_receive_answer(g_server_socket, &ans) == -1)
	{
		return -EIO;
	}
	
	if (ans.command != cmd_getexportopts)
	{
		return -EIO;
	}
	
	if (ans.ret >= 0)
	{
		*opts = (unsigned)ans.ret;
		
		DEBUG("export options: %d\n", *opts);
	}
	
	return ans.ret >= 0 ? 0 : -ans.ret_errno;
}

int _rfs_getattr(const char *path, struct stat *stbuf)
{
	if (g_server_socket == -1)
	{
		return -EIO;
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
	if (rfs_send_cmd_data(g_server_socket, &cmd, path, path_len) == -1)
	{
		return -EIO;
	}

	struct answer ans = { 0 };
	if (rfs_receive_answer(g_server_socket, &ans) == -1)
	{
		return -EIO;
	}

	if (ans.command != cmd_getattr)
	{
		return -EIO;
	}

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
	uint32_t user_len = 0;
	const char *user = NULL;
	uint32_t group_len = 0;
	const char *group = NULL;
	uint32_t size = 0;
	uint32_t atime = 0;
	uint32_t mtime = 0;
	uint32_t ctime = 0;

	unsigned last_pos = 
	unpack_32(&ctime, buffer, 
	unpack_32(&mtime, buffer, 
	unpack_32(&atime, buffer, 
	unpack_32(&size, buffer, 
	unpack_32(&group_len, buffer, 
	unpack_32(&user_len, buffer, 
	unpack_32(&mode, buffer, 0 
		)))))));
	
	user = buffer + last_pos;
	group = buffer + last_pos + user_len;
	
	if (strlen(user) + 1 != user_len
	|| strlen(group) + 1 != group_len)
	{
		free_buffer(buffer);
		return -EIO;
	}
	
	uid_t uid = (uid_t)-1;
	
	if ((export_opts & opt_ugo) != 0
	&& strcmp(rfs_config.auth_user, user) == 0)
	{
		uid = my_uid;
		user = get_uid_name(my_uid);
		if (user == NULL)
		{
			free_buffer(buffer);
			return -EINVAL;
		}
	}
	else
	{
		uid = lookup_user(user);
	}
	
	gid_t gid = lookup_group(group, user);
	
	DEBUG("user: %s, group: %s, uid: %d, gid: %d\n", user, group, uid, gid);

	free_buffer(buffer);

	struct stat result = { 0 };

	result.st_mode = mode;

	if ((export_opts & opt_ugo) == 0)
	{
		result.st_uid = getuid();
		result.st_gid = getgid();
	}
	else
	{
		result.st_uid = uid;
		result.st_gid = gid;
	}
	
	result.st_size = size;
	result.st_atime = atime;
	result.st_mtime = mtime;
	result.st_ctime = ctime;

	memcpy(stbuf, &result, sizeof(*stbuf));

	if (cache_file(path, &result) == NULL)
	{
		return -EIO;
	}

	return ans.ret == -1 ? -ans.ret_errno : ans.ret;
}

int _rfs_readdir(const char *path, void *buf, const fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
	if (g_server_socket == -1)
	{
		return -EIO;
	}

	unsigned path_len = strlen(path) + 1;

	struct command cmd = { cmd_readdir, path_len };

	if (rfs_send_cmd_data(g_server_socket, &cmd, path, path_len) == -1)
	{
		return -EIO;
	}

	struct answer ans = { 0 };
	struct stat stbuf = { 0 };
	uint32_t mode = 0;
	uint32_t user_len = 0;
	const char *user = NULL;
	uint32_t group_len = 0;
	const char *group = NULL;
	uint32_t size = 0;
	uint32_t atime = 0;
	uint32_t mtime = 0;
	uint32_t ctime = 0;
	uint16_t stat_failed = 0;

	unsigned stat_size = sizeof(mode)
	+ sizeof(user_len)
	+ MAX_SUPPORTED_NAME_LEN
	+ sizeof(group_len)
	+ MAX_SUPPORTED_NAME_LEN
	+ sizeof(size)
	+ sizeof(atime)
	+ sizeof(mtime)
	+ sizeof(ctime)
	+ sizeof(stat_failed);

	unsigned buffer_size = stat_size + NAME_MAX;
	char *buffer = get_buffer(buffer_size);
	char full_path[NAME_MAX] = { 0 };

	char operation_failed = 0;
	do
	{
		if (rfs_receive_answer(g_server_socket, &ans) == -1)
		{
			if (buffer != NULL)
			{
				free_buffer(buffer);
			}
		
			return -EIO;
		}
	
		if (ans.command != cmd_readdir)
		{
			if (buffer != NULL)
			{
				free_buffer(buffer);
			}
		
			return -EIO;
		}
	
		if (ans.ret == -1)
		{
			if (buffer != NULL)
			{
				free_buffer(buffer);
			}
		
			return -ans.ret_errno;
		}
	
		if (ans.data_len == 0)
		{
			break;
		}
	
		if (ans.data_len > buffer_size)
		{
			free_buffer(buffer);
		}
	
		memset(buffer, 0, buffer_size);
	
		if (rfs_receive_data(g_server_socket, buffer, ans.data_len) == -1)
		{
			free_buffer(buffer);
		
			return -EIO;
		}
	
		dump(buffer, ans.data_len);
	
		unsigned last_pos =
		unpack_16(&stat_failed, buffer, 
		unpack_32(&ctime, buffer, 
		unpack_32(&mtime, buffer, 
		unpack_32(&atime, buffer, 
		unpack_32(&size, buffer, 
		unpack_32(&group_len, buffer, 
		unpack_32(&user_len, buffer, 
		unpack_32(&mode, buffer, 0 
			))))))));
			
		char *entry_name = buffer + last_pos;
		unsigned entry_len = strlen(entry_name) + 1;
		
		user = buffer + last_pos + entry_len;
		group = buffer + last_pos + entry_len + user_len;
		
		if (strlen(user) + 1 != user_len
		|| strlen(group) + 1 != group_len)
		{
			free_buffer(buffer);
			return -EIO;
		}
		
		uid_t uid = (uid_t)-1;
		
		if ((export_opts & opt_ugo) != 0
		&& strcmp(rfs_config.auth_user, user) == 0)
		{
			uid = my_uid;
			user = get_uid_name(my_uid);
			if (user == NULL)
			{
				free_buffer(buffer);
				return -EINVAL;
			}
		}
		else
		{
			uid = lookup_user(user);
		}
		
		gid_t gid = lookup_group(group, user);
		
		DEBUG("user: %s, group: %s, uid: %d, gid: %d\n", user, group, uid, gid);
		
		if (stat_failed == 0)
		{
			int joined = path_join(full_path, path, entry_name);
			
			if (joined < 0)
			{
				operation_failed = 1;
				return -EIO;
			}
			
			if (joined == 0)
			{
				stbuf.st_mode = mode;
				if ((export_opts & opt_ugo) == 0)
				{
					stbuf.st_uid = getuid();
					stbuf.st_gid = getgid();
				}
				else
				{
					stbuf.st_uid = uid;
					stbuf.st_gid = gid;
				}
				stbuf.st_size = size;
				stbuf.st_atime = atime;
				stbuf.st_mtime = mtime;
				stbuf.st_ctime = ctime;
			
				if (cache_file(full_path, &stbuf) == NULL)
				{
					free_buffer(buffer);
					return -EIO;
				}
			}
		}
	
		if (operation_failed == 0)
		{
			if (filler(buf, entry_name, NULL, 0) != 0)
			{
				break;
			}
		}
	}
	while (ans.data_len > 0);

	if (buffer != NULL)
	{
		free_buffer(buffer);
	}

	return 0;
}

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

	char *buffer = get_buffer(cmd.data_len);

	pack(path, path_len, buffer, 
	pack_16(&fi_flags, buffer, 0
		));

	if (rfs_send_cmd_data(g_server_socket, &cmd, buffer, cmd.data_len) == -1)
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

	if (read_cache_is_for(fi->fh) != 0)
	{
		destroy_read_cache();
		update_read_cache_stats(-1, -1, -1);
	}

	if (write_cache_is_for(fi->fh) != 0)
	{
		destroy_write_cache();
	}

	if (get_write_cache_size() == 0)
	{
		uninit_write_cache();
	}

	uint64_t handle = htonll(fi->fh);

	struct command cmd = { cmd_release, sizeof(handle) };

	if (rfs_send_cmd_data(g_server_socket, &cmd, &handle, cmd.data_len) == -1)
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

	char *buffer = get_buffer(cmd.data_len);

	pack(path, path_len, buffer, 
	pack_32(&foffset, buffer, 0
		));

	if (rfs_send_cmd_data(g_server_socket, &cmd, buffer, cmd.data_len) == -1)
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
	size_t cached_size = read_cache_have_data(fi->fh, offset);
	if (cached_size >= size)
	{
		DEBUG("hit (%d)\n", cached_size);
		const char *cached_data = read_cache_get_data(fi->fh, size, offset);
		if (cached_data == NULL)
		{
			destroy_read_cache();
			update_read_cache_stats(-1, -1, -1);
			return -EIO;
		}
		
		memcpy(buf, cached_data, size);
		if (cached_size > size)
		{
			return size;
		}
		/* else proceed with next caching operation */
		
		DEBUG("%s\n", "need more cache");
	}
	else
	{
		cached_size = 0;
	}

	size_t cached_read_size = last_used_read_block(fi->fh);
	
	if (cached_read_size > 0)
	{
		if (cached_read_size >= size_to_read
		&& size_to_read < read_cache_max_size() / 2)
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
	}
	else
	{
		if (size_to_read * 2 < read_cache_max_size())
		{
			size_to_read = size_to_read * 2;
		}
	}
	
	unsigned overall_size = size_to_read;
	unsigned old_val = rfs_config.use_read_cache;
	rfs_config.use_read_cache = 0;

	char *buffer = read_cache_resize(overall_size);
	
	int ret = _rfs_read(path, buffer, overall_size, offset + cached_size, fi);
	rfs_config.use_read_cache = old_val;

	if (ret < 0)
	{
		destroy_read_cache();
		update_read_cache_stats(-1, -1, -1);
		return ret;
	}
	
	update_read_cache_stats(fi->fh, overall_size, offset + cached_size);

	if (cached_size < size)
	{
		memcpy(buf, buffer, ret < size ? ret : size);
		return ret == overall_size ? size : (ret >= size ? size : ret);
	}
	else
	{
		return size; /* data were copied earlier */
	}
}

int _rfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	if (g_server_socket == -1)
	{
		return -EIO;
	}

	if (rfs_config.use_read_cache          /* read_cache_max_size()/2 because we don't really need to cache */
	&& size < read_cache_max_size() / 2)   /* exactly the same data which we just read - it is most likely */
	{                                      /* will not be requested again */
		return _rfs_read_cached(path, buf, size, offset, fi);
	}

	uint64_t handle = fi->fh;
	uint32_t fsize = size;
	uint32_t foffset = offset;

#define overall_size sizeof(fsize) + sizeof(foffset) + sizeof(handle)
	struct command cmd = { cmd_read, overall_size };

	char buffer[overall_size] = { 0 };
#undef  overall_size

	pack_64(&handle, buffer, 
	pack_32(&foffset, buffer, 
	pack_32(&fsize, buffer, 0
		)));

	if (rfs_send_cmd_data(g_server_socket, &cmd, buffer, cmd.data_len) == -1)
	{
		return -EIO;
	}

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
	if (rfs_config.use_read_cache != 0
	&& read_cache_size(fi->fh) > 0)
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

	if (get_write_cache_block() == NULL)
	{
		return -EIO;
	}

	uint32_t fsize = get_write_cache_size();
	uint32_t foffset = write_cached_offset();
	uint64_t handle = write_cached_descriptor();

	unsigned old_val = rfs_config.use_write_cache;
	rfs_config.use_write_cache = 0;

	struct fuse_file_info tmp_fi = { 0 }; 	/* take a look what _rfs_write is using 
						from this struct. now it is only ->fh, 
						but be warned */
	tmp_fi.fh = handle;

	int ret = _rfs_write(path, get_write_cache_block(), fsize, foffset, &tmp_fi);

	destroy_write_cache();

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
		int ret = _rfs_flush(write_cached_path(), fi);
		if (ret < 0)
		{
			return ret;
		}
	
		size_t reinit_size = last_used_write_block();
		if (reinit_size == (size_t)-1
		|| reinit_size < size)
		{
			reinit_size = size;
		}
	
		if (reinit_size * 2 < write_cache_max_size())
		{
			if (init_write_cache(path, offset, reinit_size * 2) != 0)
			{
				return -EIO;
			}
		}
		else
		{
			if (init_write_cache(path, offset, write_cache_max_size()) != 0)
			{
				return -EIO;
			}
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

	if (rfs_config.use_write_cache != 0
	&& size < write_cache_max_size())
	{
		return _rfs_write_cached(path, buf, size, offset, fi);
	}

	uint64_t handle = fi->fh;
	uint32_t fsize = size;
	uint32_t foffset = offset;

#define header_size sizeof(fsize) + sizeof(foffset) + sizeof(handle)
	unsigned overall_size = header_size + size;
	struct command cmd = { cmd_write, overall_size };

	char buffer[header_size] = { 0 };

	pack_64(&handle, buffer, 
	pack_32(&foffset, buffer, 
	pack_32(&fsize, buffer, 0
		)));

	if (rfs_send_cmd_data(g_server_socket, &cmd, buffer, header_size) == -1)
	{
		return -EIO;
	}

#undef header_size

	if (rfs_send_data(g_server_socket, get_write_cache_block(), size) == -1)
	{
		return -EIO;
	}

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

	char *buffer = get_buffer(cmd.data_len);

	pack(path, path_len, buffer, 
	pack_32(&fmode, buffer, 0
		));

	if (rfs_send_cmd_data(g_server_socket, &cmd, buffer, cmd.data_len) == -1)
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

	if (rfs_send_cmd_data(g_server_socket, &cmd, path, cmd.data_len) == -1)
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

	if (rfs_send_cmd_data(g_server_socket, &cmd, path, cmd.data_len) == -1)
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

	char *buffer = get_buffer(cmd.data_len);

	pack(new_path, new_path_len, buffer,
	pack(path, path_len, buffer,
	pack_32(&len, buffer, 0
		)));

	if (rfs_send_cmd_data(g_server_socket, &cmd, buffer, cmd.data_len) == -1)
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

	char *buffer = get_buffer(cmd.data_len);

	pack(path, path_len, buffer, 
	pack_32(&actime, buffer, 
	pack_32(&modtime, buffer, 
	pack_16(&is_null, buffer, 0
		))));

	if (rfs_send_cmd_data(g_server_socket, &cmd, buffer, cmd.data_len) == -1)
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

	if (rfs_send_cmd_data(g_server_socket, &cmd, path, path_len) == -1)
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

	char *buffer = get_buffer(overall_size);

	pack(path, path_len, buffer, 
	pack_32(&fmode, buffer, 0
		));

	if (rfs_send_cmd_data(g_server_socket, &cmd, buffer, cmd.data_len) == -1)
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

int _rfs_chmod(const char *path, mode_t mode)
{
	if ((export_opts & opt_ugo) == 0)
	{
		return 0;
	}

	unsigned path_len = strlen(path) + 1;
	uint32_t fmode = mode;

	unsigned overall_size = sizeof(fmode) + path_len;

	struct command cmd = { cmd_chmod, overall_size };

	char *buffer = get_buffer(overall_size);
	pack(path, path_len, buffer, 
	pack_32(&fmode, buffer, 0
		));

	if (rfs_send_cmd_data(g_server_socket, &cmd, buffer, cmd.data_len) == -1)
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

	if (ans.command != cmd_chmod)
	{
		return -EIO;
	}

	struct tree_item *cached_value = get_cache(path);
	if (cached_value != NULL)
	{
		delete_from_cache(path);
	}

	return ans.ret == 0 ? 0 : -ans.ret_errno;
}

int _rfs_chown(const char *path, uid_t uid, gid_t gid)
{
	if ((export_opts & opt_ugo) == 0)
	{
		return 0;
	}

	unsigned path_len = strlen(path) + 1;
	
	const char *user = NULL;
	if (my_uid == uid)
	{
		user = rfs_config.auth_user;
	}
	else
	{
		user = get_uid_name(uid);
		if (user == NULL)
		{
			return -EINVAL;
		}
	}
	
	const char *group = NULL;
	if (my_gid == gid)
	{
		group = rfs_config.auth_user; /* yes, indeed, default group to auth_name */
	}
	else
	{
		group = get_gid_name(gid);
		if (group == NULL)
		{
			return -EINVAL;
		}
	}
	
	uint32_t user_len = strlen(user) + 1;
	uint32_t group_len = strlen(group) + 1;

	unsigned overall_size = sizeof(user_len) + sizeof(group_len) 
	+ path_len + user_len + group_len;

	struct command cmd = { cmd_chown, overall_size };

	char *buffer = get_buffer(overall_size);
	pack(group, group_len, buffer, 
	pack(user, user_len, buffer, 
	pack(path, path_len, buffer, 
	pack_32(&group_len, buffer, 
	pack_32(&user_len, buffer, 0
		)))));

	if (rfs_send_cmd_data(g_server_socket, &cmd, buffer, cmd.data_len) == -1)
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

	if (ans.command != cmd_chown)
	{
		return -EIO;
	}

	struct tree_item *cached_value = get_cache(path);
	if (cached_value != NULL)
	{
		delete_from_cache(path);
	}

	return ans.ret == 0 ? 0 : -ans.ret_errno;
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
