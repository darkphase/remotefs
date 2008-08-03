#include "server_handlers.h"

#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <utime.h>
#include <stdio.h>

#include "config.h"
#include "command.h"
#include "sendrecv.h"
#include "buffer.h"
#include "alloc.h"
#include "rfsd.h"
#include "exports.h"
#include "list.h"
#include "passwd.h"
#include "inet.h"
#include "keep_alive_server.h"
#include "crypt.h"

extern unsigned char directory_mounted;
extern struct rfsd_config rfsd_config;
extern struct rfs_export *mounted_export;
char *auth_user = NULL;
char *auth_passwd = NULL;

static char auth_salt[MAX_SALT_LEN + 1] = { 0 };

int check_password(const char *user, const char *passwd)
{
	const char *stored_passwd = get_auth_password(user);
	
	if (stored_passwd != NULL)
	{	
		char *check_crypted = passwd_hash(stored_passwd, auth_salt);
		
		DEBUG("user: %s, received passwd: %s, stored passwd: %s, salt: %s, required passwd: %s\n", user, passwd, stored_passwd, auth_salt, check_crypted);
		
		int ret = (strcmp(check_crypted, passwd) == 0 ? 0 : -1);
		free(check_crypted);
		
		return ret;
	}
	
	return -1;
}

int check_permissions(const struct rfs_export *export_info, const char *client_ip_addr)
{
	struct list *user_entry = export_info->users;
	while (user_entry != NULL)
	{
		const char *user = (const char *)user_entry->data;
		if (is_ipaddr(user) != 0)
		{
			if (strcmp(user, client_ip_addr) == 0)
			{
				DEBUG("%s\n", "access is allowed by ip address");
				return 0;
			}
		}
		else if (auth_user && auth_passwd
		&& strcmp(user, auth_user) == 0
		&& check_password(user, auth_passwd) == 0)
		{
			DEBUG("%s\n", "access is allowed by username and password");
			return 0;
		}
		
		user_entry = user_entry->next;
	}
	
	DEBUG("%s\n", "access denied");
	return -1;
}

int generate_salt(char *salt, size_t max_size)
{
	const char al_set_begin = 'a';
	const char al_set_end = 'z';
	const char alu_set_begin = 'A';
	const char alu_set_end = 'Z';
	const char num_set_begin = '0';
	const char num_set_end = '9';
	const char *additional = "./";
	
	memset(salt, 0, max_size);
	
	size_t empty_len = strlen(EMPTY_SALT);
	
	if (empty_len >= max_size)
	{
		return -1;
	}
	
	memcpy(salt, EMPTY_SALT, empty_len);
	
	enum e_set { set_al = 0, set_alu, set_num, set_additional, set_max };
	
	int i; for (i = empty_len; i < max_size; ++i)
	{
		char ch = '\0';
		
		switch (rand() % set_max)
		{
		case set_al:
			ch = al_set_begin + (rand() % (al_set_end - al_set_begin));
			break;
		case set_alu:
			ch = alu_set_begin + (rand() % (alu_set_end - alu_set_begin));
			break;
		case set_num:
			ch = num_set_begin + (rand() % (num_set_end - num_set_begin));
			break;
		case set_additional:
			ch = additional[rand() % strlen(additional)];
			break;
			
		default:
			memset(salt, 0, max_size);
			return -1;
		}
		
		DEBUG("%c\n", ch);
		
		salt[i] = ch;
	}
	
	return 0;
}

int _handle_request_salt(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	memset(auth_salt, 0, sizeof(auth_salt));
	if (generate_salt(auth_salt, sizeof(auth_salt) - 1) != 0)
	{
		return reject_request(client_socket, cmd, EREMOTEIO) == 0 ? 1 : -1;
	}
	
	uint32_t salt_len = strlen(auth_salt) + 1;
	
	struct answer ans = { cmd_request_salt, salt_len, 0, 0 };
	
	if (rfs_send_answer(client_socket, &ans) == -1)
	{
		return -1;
	}
	
	if (rfs_send_data(client_socket, auth_salt, salt_len) == -1)
	{
		return -1;
	}
	
	return 0;
}

int _handle_auth(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	if (buffer == NULL)
	{
		return -1;
	}

	if (rfs_receive_data(client_socket, buffer, cmd->data_len) == -1)
	{
		free_buffer(buffer);
		return -1;
	}
	
	uint32_t passwd_len = 0;
	
	char *passwd = buffer + 
	unpack_32(&passwd_len, buffer, 0);
	char *user = buffer + passwd_len + sizeof(passwd_len);
	
	if (strlen(user) + 1 
	+ sizeof(passwd_len) 
	+ strlen(passwd) + 1 != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(client_socket, cmd, EBADE) == 0 ? 1 : -1;
	}
		
	DEBUG("user: %s, passwd: %s, salt: %s\n", user, passwd, auth_salt);
	
	auth_user = strdup(user);
	auth_passwd = strdup(passwd);
	
	free_buffer(buffer);
	
	struct answer ans = { cmd_auth, 0, 0, 0 };
	
	if (rfs_send_answer(client_socket, &ans) == -1)
	{
		return -1;
	}
	
	return 0;
}

int _handle_closeconnection(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	DEBUG("connection to %s is closed\n", inet_ntoa(client_addr->sin_addr));
	
	server_close_connection(client_socket);
	exit(0);
}

int _handle_changepath(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	if (buffer == NULL)
	{
		return -1;
	}

	if (rfs_receive_data(client_socket, buffer, cmd->data_len) == -1)
	{
		free_buffer(buffer);
		return -1;
	}
	
	if (strlen(buffer) + 1 != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(client_socket, cmd, EBADE) == 0 ? 1 : -1;
	}
	
	while (strlen(buffer) > 1 /* do not remove first '/' */
	&& buffer[strlen(buffer) - 1] == '/')
	{
		buffer[strlen(buffer) - 1] = 0;
	}
	
	const char *path = buffer;
	
	DEBUG("client want to change path to %s\n", path);
	
	const struct rfs_export *export_info = strlen(path) > 0 ? get_export(path) : NULL;
	if (export_info == NULL 
	|| check_permissions(export_info, inet_ntoa(client_addr->sin_addr)) != 0)
	{
		free_buffer(buffer);
		return reject_request(client_socket, cmd, EACCES) == 0 ? 1 : -1;
	}

	errno = 0;
	int result = chroot(path);

	struct answer ans = { cmd_changepath, 0, result, errno };
	
	if (result == 0)
	{
		setuid(export_info->export_uid);
	}
	
	free_buffer(buffer);

	if (rfs_send_answer(client_socket, &ans) == -1)
	{
		return -1;
	}
	
	if (result == 0)
	{	
		directory_mounted = 1;
		mounted_export = get_buffer(sizeof(*mounted_export));
		memcpy(mounted_export, export_info, sizeof(*mounted_export));
		
		release_passwords();
		release_exports();
	}

	return 0;
}

int _handle_getattr(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	
	if (buffer == NULL)
	{
		return -1;	}
	
	if (rfs_receive_data(client_socket, buffer, cmd->data_len) == -1)
	{
		free_buffer(buffer);
		return -1;	}
	
	struct stat stbuf = { 0 };
	const char *path = buffer;
	
	if (strlen(path) + 1 != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(client_socket, cmd, EBADE) == 0 ? 1 : -1;
	}
	
	errno = 0;
	if (stat(path, &stbuf) != 0)
	{
		int saved_errno = errno;
		
		free_buffer(buffer);
		
		return reject_request(client_socket, cmd, saved_errno) == 0 ? 1 : -1;
	}
	
	free_buffer(buffer);
	
	uint32_t mode = stbuf.st_mode;
	uint32_t uid = stbuf.st_uid;
	uint32_t gid = stbuf.st_gid;
	uint32_t size = stbuf.st_size;
	uint32_t atime = stbuf.st_atime;
	uint32_t mtime = stbuf.st_mtime;
	uint32_t ctime = stbuf.st_ctime;
	
	unsigned overall_size = sizeof(mode)
	+ sizeof(uid)
	+ sizeof(gid)
	+ sizeof(size)
	+ sizeof(atime)
	+ sizeof(mtime)
	+ sizeof(ctime);
	
	struct answer ans = { cmd_getattr, overall_size, 0, 0 };
	
	if (rfs_send_answer(client_socket, &ans) == -1)
	{
		return -1;	}
	
	buffer = get_buffer(ans.data_len);

	pack_32(&ctime, buffer, 
	pack_32(&mtime, buffer, 
	pack_32(&atime, buffer, 
	pack_32(&size, buffer, 
	pack_32(&gid, buffer, 
	pack_32(&uid, buffer, 
	pack_32(&mode, buffer, 0
		)))))));
	
	if (rfs_send_data(client_socket, buffer, ans.data_len) == -1)
	{
		free_buffer(buffer);
		return -1;	}
	
	free_buffer(buffer);
	
	return 0;
}

int _handle_readdir(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(client_socket, buffer, cmd->data_len) == -1)
	{
		free_buffer(buffer);
		return -1;
	}

	char *path = buffer;
	
	if (strlen(path) + 1 != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(client_socket, cmd, EBADE) == 0 ? 1 : -1;
	}
	
	errno = 0;
	DIR *dir = opendir(path);
	
	struct answer ans = { cmd_readdir, 0, dir == NULL ? -1 : 0, errno };

	free_buffer(buffer);
	
	if (dir == NULL)
	{
		rfs_send_answer(client_socket, &ans);
		return 1;
	}

	struct dirent *dir_entry = NULL;
	
	{
	
	while ((dir_entry = readdir(dir)) != 0)
	{	
		struct answer ans = { cmd_readdir, strlen(dir_entry->d_name) + 1 };
		
		if (rfs_send_answer(client_socket, &ans) == -1)
		{
			closedir(dir);
			return -1;
		}
		
		if (rfs_send_data(client_socket, dir_entry->d_name, ans.data_len) == -1)
		{
			closedir(dir);
			return -1;
		}
	}

	closedir(dir);
	
	ans.data_len = 0;
	if (rfs_send_answer(client_socket, &ans) == -1)
	{
		return -1;
	}
	
	}
	
	return 0;
}

int _handle_open(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(client_socket, buffer, cmd->data_len) == -1)
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
		return reject_request(client_socket, cmd, EBADE) == 0 ? 1 : -1;
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
		if (add_file_to_open_list(fd) != 0)
		{
			close(fd);
			return reject_request(client_socket, cmd, EREMOTEIO) == 0 ? 1 : -1;
		}
	}
	
	if (rfs_send_answer(client_socket, &ans) == -1)
	{
		return -1;
	}
	
	if (ans.ret != -1)
	{
		if (rfs_send_data(client_socket, &handle, sizeof(handle)) == -1)
		{
			return -1;
		}
	}
	
	return 0;
}

int _handle_truncate(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(client_socket, buffer, cmd->data_len) == -1)
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
		return reject_request(client_socket, cmd, EBADE) == 0 ? 1 : -1;
	}
	
	errno = 0;
	int result = truncate(path, offset);
	
	struct answer ans = { cmd_truncate, 0, result, errno };
	
	free_buffer(buffer);
	
	if (rfs_send_answer(client_socket, &ans) == -1)
	{
		return -1;
	}
	
	return result == 0 ? 0 : 1;
}

int _handle_read(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(client_socket, buffer, cmd->data_len) == -1)
	{
		free_buffer(buffer);
		return -1;
	}
	
	uint64_t handle = (uint64_t)-1;
	uint32_t offset = 0;
	uint32_t size = 0;
	
	if (sizeof(handle)
	+ sizeof(offset)
	+ sizeof(size) != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(client_socket, cmd, EBADE) == 0 ? 1 : -1;
	}
	
	unpack_64(&handle, buffer, 
	unpack_32(&offset, buffer, 
	unpack_32(&size, buffer, 0
		)));
	
	DEBUG("handle: %llu, offset: %u, size: %u\n", handle, offset, size);
	
	free_buffer(buffer);
	
	if (handle == (uint64_t)-1)
	{
		return reject_request(client_socket, cmd, EBADF) == 0 ? 1 : -1;
	}
	
	int fd = (int)handle;
	
	if (lseek(fd, offset, SEEK_SET) != offset)
	{
		int saved_errno = errno;
		
		close(fd);
		
		return reject_request(client_socket, cmd, saved_errno) == 0 ? 1 : -1;
	}

	int result = 0;
	buffer = NULL;
	
	if (size > 0)
	{
		buffer = get_buffer(size);
		if (buffer == NULL)
		{
			return reject_request(client_socket, cmd, EREMOTEIO) == 0 ? 1 : -1;
		}
		
		errno = 0;
		result = read(fd, buffer, size);
		if (result == -1)
		{
			int saved_errno = errno;
			
			free_buffer(buffer);
			close(fd);
			
			return reject_request(client_socket, cmd, saved_errno) == 0 ? 1 : -1;
		}
	}
	
	if (result != -1)
	{
		struct answer ans = { cmd_read, (uint32_t)result, (int32_t)result, 0 };
		
		if (rfs_send_answer(client_socket, &ans) == -1)
		{
			if (buffer != NULL)
			{
				free_buffer(buffer);
			}
			
			return -1;
		}
		
		if (ans.data_len > 0)
		{
			if (rfs_send_data(client_socket, buffer, ans.data_len) == -1)
			{
				if (buffer != 0)
				{
					free_buffer(buffer);
				}
				
				return -1;
			}
		}
	}
	
	if (buffer != NULL)
	{
		free_buffer(buffer);
	}
	
	return 0;
}

int _handle_write(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(client_socket, buffer, cmd->data_len) == -1)
	{
		free_buffer(buffer);
		return -1;
	}
	
	uint64_t handle = (uint64_t)-1;
	uint32_t offset = 0;
	uint32_t size = 0;
	
	const char *data = buffer + 
	unpack_64(&handle, buffer, 
	unpack_32(&offset, buffer, 
	unpack_32(&size, buffer, 0
		)));
	
	if (handle == (uint64_t)-1)
	{
		free_buffer(buffer);
		
		return reject_request(client_socket, cmd, EBADF) == 0 ? 1 : -1;
	}
	
	int fd = (int)handle;
	
	if (lseek(fd, offset, SEEK_SET) != offset)
	{
		struct answer ans = { cmd_write, 0, -1, errno };
		
		free_buffer(buffer);
		close(fd);

		rfs_send_answer(client_socket, &ans);			
		return 1;
	}

	ssize_t result = 0;
	
	if (size > 0)
	{
		errno = 0;
		result = write(fd, data, size);
	}
	
	struct answer ans = { cmd_write, 0, (int32_t)result, errno };
	
	free_buffer(buffer);
	
	if (rfs_send_answer(client_socket, &ans) == -1)
	{
		return -1;
	}
	
	return result == -1 ? 1 : 0;
}

int _handle_mkdir(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(client_socket, buffer, cmd->data_len) == -1)
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
		return reject_request(client_socket, cmd, EBADE) == 0 ? 1 : -1;
	}
	
	errno = 0;
	int result = mkdir(path, mode);
	
	struct answer ans = { cmd_mkdir, 0, result, errno };
	
	free_buffer(buffer);
	
	if (rfs_send_answer(client_socket, &ans) == -1)
	{
		return -1;
	}
	
	return result != 0 ? 1 : 0;
}

int _handle_unlink(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(client_socket, buffer, cmd->data_len) == -1)
	{
		free_buffer(buffer);
		return -1;
	}
	
	const char *path = buffer;
	
	if (strlen(path) + 1 != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(client_socket, cmd, EBADE) == 0 ? 1 : -1;
	}
	
	errno = 0;
	int result = unlink(path);
	
	struct answer ans = { cmd_unlink, 0, result, errno };
	
	free_buffer(buffer);
	
	if (rfs_send_answer(client_socket, &ans) == -1)
	{
		return -1;
	}
	
	return result != 0 ? 1 : 0;
}

int _handle_rmdir(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(client_socket, buffer, cmd->data_len) == -1)
	{
		free_buffer(buffer);
		return -1;
	}
	
	const char *path = buffer;
	
	if (strlen(path) + 1 != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(client_socket, cmd, EBADE) == 0 ? 1 : -1;
	}
	
	errno = 0;
	int result = rmdir(path);
	
	struct answer ans = { cmd_rmdir, 0, result, errno };
	
	free_buffer(buffer);
	
	if (rfs_send_answer(client_socket, &ans) == -1)
	{
		return -1;
	}
	
	return result != 0 ? 1 : 0;
}

int _handle_rename(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(client_socket, buffer, cmd->data_len) == -1)
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
		return reject_request(client_socket, cmd, EBADE) == 0 ? 1 : -1;
	}
	
	errno = 0;
	int result = rename(path, new_path);
	
	struct answer ans = { cmd_rename, 0, result, errno };
	
	free_buffer(buffer);
	
	if (rfs_send_answer(client_socket, &ans) == -1)
	{
		return -1;
	}
	
	return result != 0 ? 1 : 0;
}

int _handle_utime(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(client_socket, buffer, cmd->data_len) == -1)
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
		return reject_request(client_socket, cmd, EBADE) == 0 ? 1 : -1;
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
	
	if (rfs_send_answer(client_socket, &ans) == -1)
	{
		return -1;
	}
	
	return result != 0 ? 1 : 0;
}

int _handle_statfs(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(client_socket, buffer, cmd->data_len) == -1)
	{
		free_buffer(buffer);
		return -1;
	}
	
	struct statvfs buf = { 0 };
	const char *path = buffer;
	
	if (strlen(path) + 1 != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(client_socket, cmd, EBADE) == 0 ? 1 : -1;
	}
	
	errno = 0;
	int result = statvfs(path, &buf);
	int saved_errno = errno;
	
	free_buffer(buffer);
	
	if (result != 0)
	{
		struct answer ans = { cmd_statfs, 0, -1, saved_errno };
		
		if (rfs_send_answer(client_socket, &ans) == -1)
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
	
	if (rfs_send_answer(client_socket, &ans) == -1)
	{
		return -1;
	}
	
	buffer = get_buffer(ans.data_len);
	if (buffer == NULL)
	{
		return reject_request(client_socket, cmd, EREMOTEIO) == 0 ? 1 : -1;
	}
	
	pack_32(&namemax, buffer, 
	pack_32(&ffree, buffer, 
	pack_32(&files, buffer, 
	pack_32(&bavail, buffer, 
	pack_32(&bfree, buffer, 
	pack_32(&blocks, buffer, 
	pack_32(&bsize, buffer, 0
		)))))));

	if (rfs_send_data(client_socket, buffer, ans.data_len) == -1)
	{
		free_buffer(buffer);
		return -1;
	}
	
	free_buffer(buffer);

	return 0;
}

int _handle_release(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(client_socket, buffer, cmd->data_len) == -1)
	{
		free_buffer(buffer);
		return -1;
	}
	
	uint64_t handle = (uint64_t)-1;
	unpack_64(&handle, buffer, 0);
	
	free_buffer(buffer);
	
	if (handle == (uint64_t)-1)
	{
		return reject_request(client_socket, cmd, EBADF) == 0 ? 1 : -1;
	}
	
	int fd = (int)handle;
	errno = 0;
	int ret = close(fd);
	
	struct answer ans = { cmd_release, 0, ret, errno };	
	
	if (remove_file_from_open_list(fd) != 0)
	{
		return reject_request(client_socket, cmd, EREMOTEIO) == 0 ? 1 : -1;
	}
	
	if (rfs_send_answer(client_socket, &ans) == -1)
	{
		return -1;
	}
	
	return 0;
}

int _handle_mknod(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(client_socket, buffer, cmd->data_len) == -1)
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
		return reject_request(client_socket, cmd, EBADE) == 0 ? 1 : -1;
	}
	
	errno = 0;
	int ret = mknod(path, mode, S_IFREG);
	
	struct answer ans = { cmd_mknod, 0, ret, errno };
	
	free_buffer(buffer);
	
	if (rfs_send_answer(client_socket, &ans) == -1)
	{
		return -1;
	}
	
	return 0;
}

int handle_keepalive(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	return 0;
}

#include "server_handlers_sync.c"
