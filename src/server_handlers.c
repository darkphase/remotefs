/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#ifdef WITH_IPV6
#	include <netdb.h>
#endif
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <string.h>
#include <unistd.h>
#include <utime.h>
#if defined FREEBSD
#	include <netinet/in.h>
#	include <sys/socket.h>
#	include <sys/uio.h>
#endif
#if defined QNX
#	include <sys/socket.h>
#endif
#if defined DARWIN
#	include <netinet/in.h>
#	include <sys/socket.h>
#	include <sys/uio.h>
#endif

#include "buffer.h"
#include "command.h"
#include "config.h"
#include "cleanup.h"
#include "exports.h"
#include "id_lookup.h"
#include "inet.h"
#include "instance_server.h"
#include "list.h"
#include "passwd.h"
#include "path.h"
#include "sendrecv.h"
#include "server.h"

static int stat_file(struct rfsd_instance *instance, const char *path, struct stat *stbuf)
{
	errno = 0;
#if ! defined WITH_LINKS
	if (stat(path, stbuf) != 0)
#else
	if (lstat(path, stbuf) != 0)
#endif
	{
		return errno;
	}
	
	if (instance->server.mounted_export != NULL
	&& (instance->server.mounted_export->options & OPT_RO) != 0)
	{
		stbuf->st_mode &= (~S_IWUSR);
		stbuf->st_mode &= (~S_IWGRP);
		stbuf->st_mode &= (~S_IWOTH);
	}
	
	return 0;
}

static size_t stat_size(struct rfsd_instance *instance, struct stat *stbuf, int *ret)
{
	const char *user = get_uid_name(instance->id_lookup.uids, stbuf->st_uid);
	const char *group = get_gid_name(instance->id_lookup.gids, stbuf->st_gid);
	
	if ((instance->server.mounted_export->options & OPT_UGO) != 0 
	&& (user == NULL
	|| group == NULL))
	{
		*ret = ECANCELED;
	}
	
	if (user == NULL)
	{
		user = "";
	}
	if (group == NULL)
	{
		group = "";
	}
	
	uint32_t user_len = strlen(user) + 1;
	uint32_t group_len = strlen(group) + 1;

	return 0
	+ sizeof(uint32_t) /* mode */
	+ sizeof(uint32_t) /* user_len */ 
	+ sizeof(uint32_t) /* group_len */
	+ sizeof(uint64_t) /* size */
	+ sizeof(uint64_t) /* atime */
	+ sizeof(uint64_t) /* mtime */
	+ sizeof(uint64_t) /* ctime */
	+ user_len
	+ group_len;
}

static off_t pack_stat(struct rfsd_instance *instance, char *buffer, struct stat *stbuf, int *ret)
{
	const char *user = get_uid_name(instance->id_lookup.uids, stbuf->st_uid);
	const char *group = get_gid_name(instance->id_lookup.gids, stbuf->st_gid);
	
	if ((instance->server.mounted_export->options & OPT_UGO) != 0 
	&& (user == NULL
	|| group == NULL))
	{
		*ret = ECANCELED;
	}
	
	if (user == NULL)
	{
		user = "";
	}
	if (group == NULL)
	{
		group = "";
	}
	
	DEBUG("sending user: %s, group: %s\n", user, group);
	
	uint32_t mode = stbuf->st_mode;
	uint32_t user_len = strlen(user) + 1;
	uint32_t group_len = strlen(group) + 1;
	
	uint64_t size = stbuf->st_size;
	uint64_t atime = stbuf->st_atime;
	uint64_t mtime = stbuf->st_mtime;
	uint64_t ctime = stbuf->st_ctime;

	return 
	pack(group, group_len, buffer, 
	pack(user, user_len, buffer, 
	pack_64(&ctime, buffer, 
	pack_64(&mtime, buffer, 
	pack_64(&atime, buffer, 
	pack_64(&size, buffer, 
	pack_32(&group_len, buffer, 
	pack_32(&user_len, buffer, 
	pack_32(&mode, buffer, 0
	)))))))));
}

int _handle_getattr(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(&instance->sendrecv, buffer, cmd->data_len) == -1)
	{
		free_buffer(buffer);
		return -1;
	}
	
	struct stat stbuf = { 0 };
	const char *path = buffer;
	
	if (strlen(path) + 1 != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	errno = stat_file(instance, path, &stbuf);
	if (errno != 0)
	{
		int saved_errno = errno;
		
		free_buffer(buffer);
		return reject_request(instance, cmd, saved_errno) == 0 ? 1 : -1;
	}
	
	free_buffer(buffer);
	
	int size_ret = 0;
	unsigned overall_size = stat_size(instance, &stbuf, &size_ret);

	if (size_ret != 0)
	{
		return reject_request(instance, cmd, size_ret) == 0 ? 1: -1;
	}
	
	struct answer ans = { cmd_getattr, overall_size, 0, 0 };

	buffer = get_buffer(ans.data_len);

	int pack_ret = 0;
	pack_stat(instance, buffer, &stbuf, &pack_ret);

	if (pack_ret != 0)
	{
		return reject_request(instance, cmd, pack_ret) == 0 ? 1: -1;
	}
	
	if (rfs_send_answer_data(&instance->sendrecv, &ans, buffer, ans.data_len) == -1)
	{
		free_buffer(buffer);
		return -1;
	}

	free_buffer(buffer);
	
	return 0;
}

int _handle_readdir(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(&instance->sendrecv, buffer, cmd->data_len) == -1)
	{
		free_buffer(buffer);
		return -1;
	}

	char *path = buffer;
	unsigned path_len = strlen(path) + 1;
	
	if (path_len != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	if (path_len > FILENAME_MAX)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	errno = 0;
	DIR *dir = opendir(path);
	
	if (dir == NULL)
	{
		int saved_errno = errno;
		
		free_buffer(path);
		return reject_request(instance, cmd, saved_errno) == 0 ? 1 : -1;
	}

	struct dirent *dir_entry = NULL;
	struct stat stbuf = { 0 };
	uint16_t stat_failed = 0;
	
	char full_path[FILENAME_MAX + 1] = { 0 };
	
	struct answer ans = { cmd_readdir, 0 };
	
	while ((dir_entry = readdir(dir)) != 0)
	{	
		const char *entry_name = dir_entry->d_name;
		unsigned entry_len = strlen(entry_name) + 1;
		
		stat_failed = 0;
		memset(&stbuf, 0, sizeof(stbuf));
		
		int joined = path_join(full_path, sizeof(full_path), path, entry_name);
		if (joined < 0)
		{
			stat_failed = 1;
		}
	
		if (joined == 0)
		{
			if (stat_file(instance, full_path, &stbuf) != 0)
			{
				stat_failed = 1;
			}
		}
		
		int size_ret = 0;
		size_t stat_struct_size = stat_size(instance, &stbuf, &size_ret);

		if (size_ret != 0)
		{
			stat_failed = 1;
		}
		
		unsigned overall_size = stat_struct_size + sizeof(stat_failed) + entry_len;
		buffer = get_buffer(overall_size);

		ans.data_len = overall_size;
		
		int pack_ret = 0; 
		off_t last_offset = pack_stat(instance, buffer, &stbuf, &pack_ret);

		if (pack_ret != 0)
		{
			stat_failed = 1;
		}

		pack(entry_name, entry_len, buffer, 
		pack_16(&stat_failed, buffer, last_offset
		));
		
		dump(buffer, overall_size);

		if (rfs_send_answer_data(&instance->sendrecv, &ans, buffer, ans.data_len) == -1)
		{
			closedir(dir);
			free_buffer(path);
			free_buffer(buffer);
			return -1;
		}
	}

	closedir(dir);
	free_buffer(path);
	free_buffer(buffer);
	
	ans.data_len = 0;
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return 0;
}

int _handle_open(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(&instance->sendrecv, buffer, cmd->data_len) == -1)
	{
		free_buffer(buffer);
		return -1;
	}
	
	uint16_t fi_flags = 0;
	const char *path = buffer +
	unpack_16(&fi_flags, buffer, 0);
	
	if (sizeof(fi_flags) 
	+ strlen(path) + 1 != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	int flags = 0;
	if (fi_flags & RFS_APPEND) { flags |= O_APPEND; }
	if (fi_flags & RFS_ASYNC) { flags |= O_ASYNC; }
	if (fi_flags & RFS_CREAT) { flags |= O_CREAT; }
	if (fi_flags & RFS_EXCL) { flags |= O_EXCL; }
	if (fi_flags & RFS_NONBLOCK) { flags |= O_NONBLOCK; }
	if (fi_flags & RFS_NDELAY) { flags |= O_NDELAY; }
	if (fi_flags & RFS_SYNC) { flags |= O_SYNC; }
	if (fi_flags & RFS_TRUNC) { flags |= O_TRUNC; }
	if (fi_flags & RFS_RDONLY) { flags |= O_RDONLY; }
	if (fi_flags & RFS_WRONLY) { flags |= O_WRONLY; }
	if (fi_flags & RFS_RDWR) { flags |= O_RDWR; }
	
	errno = 0;
	int fd = open(path, flags);
	uint64_t handle = htonll((uint64_t)fd);
	
	struct answer ans = { cmd_open, sizeof(handle), fd == -1 ? -1 : 0, errno };
	
	free_buffer(buffer);
	
	if (fd != -1)
	{
		if (add_file_to_open_list(instance, fd) != 0)
		{
			close(fd);
			return reject_request(instance, cmd, ECANCELED) == 0 ? 1 : -1;
		}
	}

	if (ans.ret == -1)
	{
		if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
		{
			return -1;
		}
	}
	else
	{
		if (rfs_send_answer_data(&instance->sendrecv, &ans, &handle, sizeof(handle)) == -1)
		{
			return -1;
		}
	}

	return 0;
}

int _handle_truncate(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(&instance->sendrecv, buffer, cmd->data_len) == -1)
	{
		free_buffer(buffer);
		return -1;
	}
	
	uint32_t offset = (uint32_t)-1;
	const char *path = buffer +
	unpack_32(&offset, buffer, 0);
	
	if (sizeof(offset)
	+ strlen(path) + 1 != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	errno = 0;
	int result = truncate(path, offset);
	
	struct answer ans = { cmd_truncate, 0, result, errno };
	
	free_buffer(buffer);
	
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return result == 0 ? 0 : 1;
}

int _handle_mkdir(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(&instance->sendrecv, buffer, cmd->data_len) == -1)
	{
		free_buffer(buffer);
		return -1;
	}
	
	uint32_t mode = 0;
	
	const char *path = buffer + 
	unpack_32(&mode, buffer, 0);
	
	if (sizeof(mode)
	+ strlen(path) + 1 != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	errno = 0;
	int result = mkdir(path, mode);
	
	struct answer ans = { cmd_mkdir, 0, result, errno };
	
	free_buffer(buffer);
	
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return result != 0 ? 1 : 0;
}

int _handle_unlink(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(&instance->sendrecv, buffer, cmd->data_len) == -1)
	{
		free_buffer(buffer);
		return -1;
	}
	
	const char *path = buffer;
	
	if (strlen(path) + 1 != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	errno = 0;
	int result = unlink(path);
	
	struct answer ans = { cmd_unlink, 0, result, errno };
	
	free_buffer(buffer);
	
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return result != 0 ? 1 : 0;
}

int _handle_rmdir(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(&instance->sendrecv, buffer, cmd->data_len) == -1)
	{
		free_buffer(buffer);
		return -1;
	}
	
	const char *path = buffer;
	
	if (strlen(path) + 1 != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	errno = 0;
	int result = rmdir(path);
	
	struct answer ans = { cmd_rmdir, 0, result, errno };
	
	free_buffer(buffer);
	
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return result != 0 ? 1 : 0;
}

int _handle_rename(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(&instance->sendrecv, buffer, cmd->data_len) == -1)
	{
		free_buffer(buffer);
		return -1;
	}
	
	uint32_t len = 0;
	const char *path = buffer + 
	unpack_32(&len, buffer, 0);
	
	const char *new_path = buffer + sizeof(len) + len;
	
	if (sizeof(len)
	+ strlen(path) + 1
	+ strlen(new_path) + 1 != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	errno = 0;
	int result = rename(path, new_path);
	
	struct answer ans = { cmd_rename, 0, result, errno };
	
	free_buffer(buffer);
	
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return result != 0 ? 1 : 0;
}

int _handle_utime(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(&instance->sendrecv, buffer, cmd->data_len) == -1)
	{
		free_buffer(buffer);
		return -1;
	}
	
	uint16_t is_null = 0;
	uint32_t modtime = 0;
	uint32_t actime = 0;	
	const char *path = buffer + 
	unpack_32(&actime, buffer, 
	unpack_32(&modtime, buffer, 
	unpack_16(&is_null, buffer, 0
		)));
	
	if (sizeof(actime)
	+ sizeof(modtime)
	+ sizeof(is_null)
	+ strlen(path) + 1 != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	struct utimbuf *buf = NULL;
	
	if (is_null != 0)
	{
		buf = get_buffer(sizeof(*buf));
		buf->modtime = modtime;
		buf->actime = actime;
	}
	
	errno = 0;
	int result = utime(path, buf);
	
	struct answer ans = { cmd_utime, 0, result, errno };
	
	if (buf != NULL)
	{
		free_buffer(buf);
	}
	
	free_buffer(buffer);
	
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return result != 0 ? 1 : 0;
}

int _handle_statfs(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(&instance->sendrecv, buffer, cmd->data_len) == -1)
	{
		free_buffer(buffer);
		return -1;
	}
	
	struct statvfs buf = { 0 };
	const char *path = buffer;
	
	if (strlen(path) + 1 != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	errno = 0;
	int result = statvfs(path, &buf);
	int saved_errno = errno;
	
	free_buffer(buffer);
	
	if (result != 0)
	{
		struct answer ans = { cmd_statfs, 0, -1, saved_errno };
		
		if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
		{
			return -1;
		}
		
		return 1;
	}
	
	uint32_t bsize = buf.f_bsize;
	uint32_t blocks = buf.f_blocks;
	uint32_t bfree = buf.f_bfree;
	uint32_t bavail = buf.f_bavail;
	uint32_t files = buf.f_files;
	uint32_t ffree = buf.f_bfree;
	uint32_t namemax = buf.f_namemax;
	
	unsigned overall_size = sizeof(bsize)
	+ sizeof(blocks)
	+ sizeof(bfree)
	+ sizeof(bavail)
	+ sizeof(files)
	+ sizeof(ffree)
	+ sizeof(namemax);
	
	struct answer ans = { cmd_statfs, overall_size, 0, 0 };

	buffer = get_buffer(ans.data_len);
	if (buffer == NULL)
	{
		return reject_request(instance, cmd, ECANCELED) == 0 ? 1 : -1;
	}
	
	pack_32(&namemax, buffer, 
	pack_32(&ffree, buffer, 
	pack_32(&files, buffer, 
	pack_32(&bavail, buffer, 
	pack_32(&bfree, buffer, 
	pack_32(&blocks, buffer, 
	pack_32(&bsize, buffer, 0
		)))))));

	if (rfs_send_answer_data(&instance->sendrecv, &ans, buffer, ans.data_len) == -1)
	{
		free_buffer(buffer);
		return -1;
	}

	free_buffer(buffer);

	return 0;
}

int _handle_release(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(&instance->sendrecv, buffer, cmd->data_len) == -1)
	{
		free_buffer(buffer);
		return -1;
	}
	
	uint64_t handle = (uint64_t)-1;
	unpack_64(&handle, buffer, 0);
	
	free_buffer(buffer);
	
	if (handle == (uint64_t)-1)
	{
		return reject_request(instance, cmd, EBADF) == 0 ? 1 : -1;
	}
	
	int fd = (int)handle;
	errno = 0;
	int ret = close(fd);
	
	struct answer ans = { cmd_release, 0, ret, errno };	
	
	if (remove_file_from_open_list(instance, fd) != 0)
	{
		return reject_request(instance, cmd, ECANCELED) == 0 ? 1 : -1;
	}
	
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return 0;
}

int _handle_chmod(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(&instance->sendrecv, buffer, cmd->data_len) == -1)
	{
		free_buffer(buffer);
		return -1;
	}
	
	uint32_t mode = 0;
	const char *path = buffer +
	unpack_32(&mode, buffer, 0);
	
	if (sizeof(mode) + strlen(path) + 1 != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	errno = 0;
	int ret = chmod(path, mode);
	
	struct answer ans = { cmd_chmod, 0, ret, errno };
	
	free_buffer(buffer);
	
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return 0;
}

int _handle_chown(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(&instance->sendrecv, buffer, cmd->data_len) == -1)
	{
		free_buffer(buffer);
		return -1;
	}
	
	uint32_t user_len = 0;
	uint32_t group_len = 0;
	unsigned last_pos =
	unpack_32(&group_len, buffer, 
	unpack_32(&user_len, buffer, 0
	));
	
	const char *path = buffer + last_pos;
	unsigned path_len = strlen(path) + 1;
	
	const char *user = buffer + last_pos + path_len;
	const char *group = buffer + last_pos + path_len + user_len;

	if (sizeof(user_len) + sizeof(group_len)
	+ path_len + user_len + group_len != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	uid_t uid = (strlen(user) == 0 ? - 1 : get_uid(instance->id_lookup.uids, user));
	gid_t gid = (strlen(group) == 0 ? -1 : get_gid(instance->id_lookup.gids, group));
	
	if (uid == -1 
	&& gid == -1) /* if both == -1, then this is error. however one of them == -1 is ok */
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	errno = 0;
	int ret = chown(path, uid, gid);
	
	struct answer ans = { cmd_chown, 0, ret, errno };
	
	free_buffer(buffer);
	
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return 0;
}

int _handle_mknod(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(&instance->sendrecv, buffer, cmd->data_len) == -1)
	{
		free_buffer(buffer);
		return -1;
	}
	
	uint32_t mode = 0;
	const char *path = buffer +
	unpack_32(&mode, buffer, 0);
	
	if (sizeof(mode)
	+ strlen(path) + 1 != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	errno = 0;
	int ret = creat(path, mode & 0777);
	if ( ret != -1 )
	{
		close(ret);
	}
	
	struct answer ans = { cmd_mknod, 0, ret, errno };
	
	free_buffer(buffer);
	
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return 0;
}

int _handle_lock(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	uint16_t flags = 0;
	uint16_t type = 0;
	uint16_t whence = 0;
	uint64_t start = 0;
	uint64_t len = 0;
	uint64_t fd = 0;
	
#define overall_size sizeof(fd) + sizeof(flags) + sizeof(type) + sizeof(whence) + sizeof(start) + sizeof(len)
	if (cmd->data_len != overall_size)
	{
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}

	char buffer[overall_size] = { 0 };
	
	if (rfs_receive_data(&instance->sendrecv, buffer, overall_size) == -1)
	{
		return -1;
	}
#undef overall_size

	unpack_64(&len, buffer,
	unpack_64(&start, buffer,
	unpack_16(&whence, buffer, 
	unpack_16(&type, buffer,
	unpack_16(&flags, buffer,
	unpack_64(&fd, buffer, 0
	))))));
	
	int lock_cmd = 0;
	switch (flags)
	{
	case RFS_GETLK:
		lock_cmd = F_GETLK;
		break;
	case RFS_SETLK:
		lock_cmd = F_SETLK;
		break;
	case RFS_SETLKW:
		lock_cmd = F_SETLKW;
		break;
	default:
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	short lock_type = 0;
	switch (type)
	{
	case RFS_UNLCK:
		lock_type = F_UNLCK;
		break;
	case RFS_RDLCK:
		lock_type = F_RDLCK;
		break;
	case RFS_WRLCK:
		lock_type = F_WRLCK;
		break;
	default:
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	DEBUG("lock command: %d (%d)\n", lock_cmd, flags);
	DEBUG("lock type: %d (%d)\n", lock_type, type);
	
	struct flock fl = { 0 };
	fl.l_type = lock_type;
	fl.l_whence = (short)whence;
	fl.l_start = (off_t)start;
	fl.l_len = (off_t)len;
	fl.l_pid = getpid();
	
	errno = 0;
	int ret = fcntl((int)fd, lock_cmd, &fl);
	
	struct answer ans = { cmd_lock, 0, ret, errno };
	
	if (errno == 0)
	{
		if (lock_type == F_UNLCK)
		{
			remove_file_from_locked_list(instance, (int)fd);
		}
		else if (lock_type == F_RDLCK || lock_type == F_WRLCK)
		{
			add_file_to_locked_list(instance, (int)fd);
		}
	}
	
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return 0;
}

