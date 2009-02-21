/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#if defined FREEBSD
#	include <netinet/in.h>
#	include <sys/uio.h>
#	include <sys/socket.h>
#endif
#if defined QNX
#       include <sys/socket.h>
#endif
#if defined DARWIN
#	include <netinet/in.h>
#	include <sys/uio.h>
#	include <sys/socket.h>
extern int     sendfile(int, int, off_t, size_t,  void *, off_t *, int);
#endif
#if ! defined FREEBSD && ! defined DARWIN && ! defined QNX
#	include <sys/sendfile.h>
#endif
#ifdef WITH_IPV6
#	include <netdb.h>
#endif
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <utime.h>
#include <pwd.h>
#include <grp.h>

#include "config.h"
#include "server_handlers.h"
#include "command.h"
#include "sendrecv.h"
#include "buffer.h"
#include "exports.h"
#include "list.h"
#include "passwd.h"
#include "inet.h"
#include "keep_alive_server.h"
#include "crypt.h"
#include "path.h"
#include "id_lookup.h"
#include "sockets.h"
#include "cleanup.h"
#include "utils.h"
#include "instance.h"
#include "server.h"

#ifdef WITH_SSL
#include "ssl.h"
#endif

#include "server_handlers_read.c"

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

static int check_password(struct rfsd_instance *instance)
{
	const char *stored_passwd = get_auth_password(instance->passwd.auths, instance->server.auth_user);
	
	if (stored_passwd != NULL)
	{	
		char *check_crypted = passwd_hash(stored_passwd, instance->server.auth_salt);
		
		DEBUG("user: %s, received passwd: %s, stored passwd: %s, salt: %s, required passwd: %s\n", instance->server.auth_user, instance->server.auth_passwd, stored_passwd, instance->server.auth_salt, check_crypted);
		
		int ret = (strcmp(check_crypted, instance->server.auth_passwd) == 0 ? 0 : -1);
		free(check_crypted);
		
		return ret;
	}
	
	return -1;
}

static int check_permissions(struct rfsd_instance *instance, const struct rfs_export *export_info, const char *client_ip_addr)
{
	struct list *user_entry = export_info->users;
	while (user_entry != NULL)
	{
		const char *user = (const char *)user_entry->data;
		if ((export_info->options & OPT_UGO) == 0
		&& is_ipaddr(user) != 0)
		{
#ifndef WITH_IPV6
			if (strcmp(user, client_ip_addr) == 0)
			{
				DEBUG("%s\n", "access is allowed by ip address");
				return 0;
			}
#else
			if ( strcmp(user, client_ip_addr) == 0 ||
			     (strncmp(client_ip_addr, "::ffff:", 7) == 0 &&
			      strcmp(user, client_ip_addr+7) == 0)
			   )
			{
				DEBUG("%s\n", "access is allowed by ip address");
				return 0;
			}
#endif
		}
		else if (instance->server.auth_user != NULL
		&& instance->server.auth_passwd != NULL
		&& strcmp(user, instance->server.auth_user) == 0
		&& check_password(instance) == 0)
		{
			DEBUG("%s\n", "access is allowed by username and password");
			return 0;
		}
		
		user_entry = user_entry->next;
	}
	
	DEBUG("%s\n", "access denied");
	return -1;
}

static int generate_salt(char *salt, size_t max_size)
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
		
		salt[i] = ch;
	}
	
	return 0;
}

int handle_keepalive(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	/* no need of actual handling, 
	rfsd will update keep-alive on each operation anyway */
	return 0;
}

static int _handle_request_salt(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	memset(instance->server.auth_salt, 0, sizeof(instance->server.auth_salt));
	if (generate_salt(instance->server.auth_salt, sizeof(instance->server.auth_salt) - 1) != 0)
	{
		return reject_request(instance, cmd, ECANCELED) == 0 ? 1 : -1;
	}
	
	uint32_t salt_len = strlen(instance->server.auth_salt) + 1;
	
	struct answer ans = { cmd_request_salt, salt_len, 0, 0 };
	
	if (rfs_send_answer_data(&instance->sendrecv, &ans, instance->server.auth_salt, salt_len) == -1)
	{
		return -1;
	}

	return 0;
}

static int _handle_auth(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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
	
	uint32_t passwd_len = 0;
	
	char *passwd = buffer + 
	unpack_32(&passwd_len, buffer, 0);
	char *user = buffer + passwd_len + sizeof(passwd_len);
	
	if (strlen(user) + 1 
	+ sizeof(passwd_len) 
	+ strlen(passwd) + 1 != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
		
	DEBUG("user: %s, passwd: %s, salt: %s\n", user, passwd, instance->server.auth_salt);
	
	instance->server.auth_user = strdup(user);
	instance->server.auth_passwd = strdup(passwd);
	
	free_buffer(buffer);
	
	struct answer ans = { cmd_auth, 0, 0, 0 };
	
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return 0;
}

static int _handle_closeconnection(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
#ifndef WITH_IPV6
	DEBUG("connection to %s is closed\n", inet_ntoa(client_addr->sin_addr));
#else
#ifdef RFS_DEBUG
	const struct sockaddr_in6 *sa6 = (struct sockaddr_in6*)client_addr;
	char straddr[INET6_ADDRSTRLEN];
	if (client_addr->sin_family == AF_INET)
	{
		inet_ntop(AF_INET, &client_addr->sin_addr, straddr,sizeof(straddr));
	}
	else
	{
		inet_ntop(AF_INET6, &sa6->sin6_addr, straddr,sizeof(straddr));
	}
#endif
	DEBUG("connection to %s is closed\n", straddr);

#endif
	release_rfsd_instance(instance);
	server_close_connection(instance);
	exit(0);
}

static int init_groups_for_ugo(struct rfsd_instance *instance, gid_t user_gid)
{
	/* we have to init groups before chroot() */
	DEBUG("initing groups for %s\n", instance->server.auth_user);
	errno = 0;
	if (initgroups(instance->server.auth_user, user_gid) != 0)
	{
		return -errno;
	}
#ifdef RFS_DEBUG
	gid_t user_groups[255] = { 0 };
	int user_groups_count = getgroups(sizeof(user_groups) / sizeof(*user_groups), user_groups);
	if (user_groups_count > 0)
	{
		int i = 0; for (i = 0; i < user_groups_count; ++i)
		{
			struct group *grp = getgrgid(user_groups[i]);
			if (grp != NULL)
			{
				DEBUG("member of %s\n", grp->gr_name);
			}
		}
	}
#endif
	return 0;
}

static int setup_export_opts(struct rfsd_instance *instance, const struct rfs_export *export_info, uid_t user_uid, gid_t user_gid)
{
	/* always set gid first :)
	*/
	
	if ((export_info->options & OPT_UGO) != 0)
	{
		DEBUG("setting process ids according to UGO. uid: %d, gid: %d\n", user_uid, user_gid);
		if (instance->server.auth_user != NULL)
		{
			errno = 0;
#if ! defined DARWIN
			if (setregid(user_gid, user_gid) != 0
			|| setreuid(user_uid, user_uid) != 0)
#else
			if( setgid(user_gid) != 0
			|| setuid(user_uid) != 0)
#endif
			{
				return -errno;
			}
		}
	}
	else
	{
		DEBUG("setting process ids according to user= and group=. uid: %d, gid: %d\n", export_info->export_uid, export_info->export_gid);
		if (export_info->export_gid != getgid() 
		&& export_info->export_gid != (gid_t)(-1))
		{
			errno = 0;
			if (setregid(export_info->export_gid,
			export_info->export_gid) != 0)
			{
				return -errno;
			}
		}
		
		if (export_info->export_uid != getuid()
		&& export_info->export_gid != (gid_t)(-1))
		{
			errno = 0;
			if (setreuid(export_info->export_uid,
			export_info->export_uid) != 0)
			{
				return -errno;
			}
		}
	}
	
	return 0;
}

static int _handle_changepath(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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
	
	if (strlen(buffer) + 1 != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	while (strlen(buffer) > 1 /* do not remove first '/' */
	&& buffer[strlen(buffer) - 1] == '/')
	{
		buffer[strlen(buffer) - 1] = 0;
	}
	
	const char *path = buffer;
	
	DEBUG("client want to change path to %s\n", path);

	const struct rfs_export *export_info = strlen(path) > 0 ? get_export(instance->exports.list, path) : NULL;
#ifndef WITH_IPV6
	if (export_info == NULL 
	|| check_permissions(instance, export_info, inet_ntoa(client_addr->sin_addr)) != 0)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EACCES) == 0 ? 1 : -1;
	}
#else
	const struct sockaddr_in6 *sa6 = (struct sockaddr_in6*)client_addr;
	char straddr[INET6_ADDRSTRLEN];
	if (client_addr->sin_family == AF_INET)
	{
		inet_ntop(AF_INET, &client_addr->sin_addr, straddr,sizeof(straddr));
	}
	else
	{
		inet_ntop(AF_INET6, &sa6->sin6_addr, straddr,sizeof(straddr));
	}
	
	if (export_info == NULL 
	|| check_permissions(instance, export_info, straddr) != 0) 
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EACCES) == 0 ? 1 : -1;
	}
#endif
	/* the server must known oir identity (usi/primary gid
	 * this is important in order to have the correct right
	 * managment on the server. We assume here that all systems
	 * within our network know the same user and groups and the
	 * the corresponding uid/gid are the same
	 */
	uid_t user_uid = geteuid();
	gid_t user_gid = getegid();
	
	DEBUG("auth user: %s, ugo: %d\n", instance->server.auth_user, export_info->options & OPT_UGO);
	
	if (instance->server.auth_user != NULL 
	&& (export_info->options & OPT_UGO) != 0)
	{
		struct passwd *pwd = getpwnam(instance->server.auth_user);
		if (pwd != NULL)
		{
			user_uid = pwd->pw_uid;
			user_gid = pwd->pw_gid;
		}
		else
		{
			free_buffer(buffer);
			return reject_request(instance, cmd, EACCES) == 0 ? 1 : -1;
		}
	}
	
	if (instance->server.auth_user != NULL
	&& (export_info->options & OPT_UGO) != 0)
	{
		int init_errno = init_groups_for_ugo(instance, user_gid);
		if (init_errno != 0)
		{
			free_buffer(buffer);
			return reject_request(instance, cmd, -init_errno) == 0 ? 1 : -1;
		}
		
		create_uids_lookup(&instance->id_lookup.uids);
		create_gids_lookup(&instance->id_lookup.gids);
	}
	
	errno = 0;
	int result = chroot(path);

	struct answer ans = { cmd_changepath, 0, result, errno };
	
	free_buffer(buffer);
	
	/* there is a bug here or above. we can go here with export_info == NULL */
	/* [alex] we shouldn't according to if (export_info == NULL ... above
	if we are, then *that* bug should be fixed. i'll check it */
	if (result == 0)
	{
		int setup_errno = setup_export_opts(instance, export_info, user_uid, user_gid);
		if (setup_errno != 0)
		{
			destroy_uids_lookup(&instance->id_lookup.uids);
			destroy_gids_lookup(&instance->id_lookup.gids);
			
			return reject_request(instance, cmd, -setup_errno) == 0 ? 1 : -1;
		}
	}

	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	if (result == 0)
	{	
		instance->server.directory_mounted = 1;
		instance->server.mounted_export = get_buffer(sizeof(*instance->server.mounted_export));
		memcpy(instance->server.mounted_export, export_info, sizeof(*instance->server.mounted_export));
		
		release_passwords(&instance->passwd.auths);
		release_exports(&instance->exports.list);
	}
	else
	{
		destroy_uids_lookup(&instance->id_lookup.uids);
		destroy_gids_lookup(&instance->id_lookup.gids);
	}

	return 0;
}

static int _handle_getexportopts(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	struct answer ans = { cmd_getexportopts, 
	0,
	(instance->server.mounted_export != NULL ? instance->server.mounted_export->options : -1),
	(instance->server.mounted_export != NULL ? 0 : EACCES) };

	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return (instance->server.mounted_export != NULL ? 0 : 1);
}

int handle_setsocktimeout(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	int32_t timeout;
#define overall_size sizeof(timeout)
	char buffer[overall_size] = { 0 };

	if (rfs_receive_data(&instance->sendrecv, buffer, overall_size) == -1)
	{
		return -1;
	}
#undef overall_size

	unpack_32_s(&timeout, buffer, 0);
	
	DEBUG("client requested to set socket timeout to %d\n", timeout);
	
	int ret = setup_socket_timeout(instance->sendrecv.socket, (int)timeout);
	
	struct answer ans = { cmd_setsocktimeout, 0, ret == 0 ? 0 : -1, ret };
	
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return 0;
}

int handle_setsockbuffer(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	int32_t buffer_size;
#define overall_size sizeof(buffer_size)
	char buffer[overall_size] = { 0 };

	if (rfs_receive_data(&instance->sendrecv, buffer, overall_size) == -1)
	{
		return -1;
	}
#undef overall_size

	unpack_32_s(&buffer_size, buffer, 0);
	
	DEBUG("client requested to set socket buffer to %d\n", buffer_size);
	
	int ret = setup_socket_buffer(instance->sendrecv.socket, (int)buffer_size);
	
	struct answer ans = { cmd_setsockbuffer, 0, ret == 0 ? 0 : -1, ret };
	
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return 0;
}

static int _handle_getattr(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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
	
	uint32_t mode = stbuf.st_mode;
	
	const char *user = get_uid_name(instance->id_lookup.uids, stbuf.st_uid);
	const char *group = get_gid_name(instance->id_lookup.gids, stbuf.st_gid);
	
	if ((instance->server.mounted_export->options & OPT_UGO) != 0 
	&& (user == NULL
	|| group == NULL))
	{
		return reject_request(instance, cmd, ECANCELED) == 0 ? 1 : -1;
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
	
	uint32_t user_len = strlen(user) + 1;
	uint32_t group_len = strlen(group) + 1;
	
	uint64_t size = stbuf.st_size;
	uint64_t atime = stbuf.st_atime;
	uint64_t mtime = stbuf.st_mtime;
	uint64_t ctime = stbuf.st_ctime;

	unsigned overall_size = sizeof(mode)
	+ sizeof(user_len)
	+ user_len
	+ sizeof(group_len)
	+ group_len
	+ sizeof(size)
	+ sizeof(atime)
	+ sizeof(mtime)
	+ sizeof(ctime);
	
	struct answer ans = { cmd_getattr, overall_size, 0, 0 };

	buffer = get_buffer(ans.data_len);

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

	if (rfs_send_answer_data(&instance->sendrecv, &ans, buffer, ans.data_len) == -1)
	{
		free_buffer(buffer);
		return -1;
	}

	free_buffer(buffer);
	
	return 0;
}

static int _handle_readdir(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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
	uint32_t mode = 0;
	uint32_t user_len = 0;
	const char *user = NULL;
	uint32_t group_len = 0;
	const char *group = NULL;
	uint64_t size = 0;
	uint64_t atime = 0;
	uint64_t mtime = 0;
	uint64_t ctime = 0;
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
	
	char full_path[FILENAME_MAX] = { 0 };
	buffer = get_buffer(stat_size + sizeof(full_path));
	
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
		
		unsigned overall_size = stat_size + entry_len;
		
		if (joined == 0)
		{
			if (stat_file(instance, full_path, &stbuf) != 0)
			{
				stat_failed = 1;
			}
		}
		
		mode = stbuf.st_mode;
		
		user = get_uid_name(instance->id_lookup.uids, stbuf.st_uid);
		if (user == NULL)
		{
			stat_failed = 1;
			user = "";
		}
		user_len = strlen(user) + 1;
		
		if (user_len > MAX_SUPPORTED_NAME_LEN)
		{
			stat_failed = 1;
			user = "";
			user_len = 1;
		}
		
		group = get_gid_name(instance->id_lookup.gids, stbuf.st_gid);
		if (group == NULL)
		{
			stat_failed = 1;
			group = "";
		}
		group_len = strlen(group) + 1;
		
		if (group_len > MAX_SUPPORTED_NAME_LEN)
		{
			stat_failed = 1;
			group = "";
			group_len = 1;
		}
		
		if (user == NULL 
		|| group == NULL)
		{
			stat_failed = 1;
		}
		
		size = stbuf.st_size;
		atime = stbuf.st_atime;
		mtime = stbuf.st_mtime;
		ctime = stbuf.st_ctime;
		
		DEBUG("sending user: %s, group: %s\n", user, group);
		
		ans.data_len = overall_size;
		
		pack(entry_name, entry_len, buffer, 
		pack_16(&stat_failed, buffer, 
		pack(group, group_len, buffer, 
		pack(user, user_len, buffer, 
		pack_64(&ctime, buffer, 
		pack_64(&mtime, buffer, 
		pack_64(&atime, buffer, 
		pack_64(&size, buffer, 
		pack_32(&group_len, buffer, 
		pack_32(&user_len, buffer, 
		pack_32(&mode, buffer, 0
			)))))))))));
		
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

static int _handle_open(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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

static int _handle_truncate(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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

static int _handle_write(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	uint64_t handle = (uint64_t)-1;
	uint64_t offset = 0;
	uint32_t size = 0;
	
#define header_size sizeof(handle) + sizeof(offset) + sizeof(size)
	char buffer[header_size] = { 0 };
	if (rfs_receive_data(&instance->sendrecv, buffer, header_size) == -1)
	{
		return -1;
	}
#undef header_size
	
	unpack_64(&handle, buffer, 
	unpack_64(&offset, buffer, 
	unpack_32(&size, buffer, 0
		)));
	
	if (handle == (uint64_t)-1)
	{
		return reject_request(instance, cmd, EBADF) == 0 ? 1 : -1;
	}
	
	DEBUG("handle: %llu, offset: %llu, size: %u\n", (unsigned long long)handle, (unsigned long long)offset, size);

	int fd = (int)handle;
	
	if (lseek(fd, offset, SEEK_SET) != offset)
	{
		if (rfs_ignore_incoming_data(&instance->sendrecv, size) == -1)
		{
			return -1;
		}
		
		return reject_request(instance, cmd, ECANCELED) == 0 ? 1 : -1;
	}
	
	size_t done = 0;
	errno = 0;
	int saved_errno = errno;
	
	char data[RFS_WRITE_BLOCK] = { 0 };
	
	while (done < size)
	{
		unsigned current_block_size = (size - done >= RFS_WRITE_BLOCK) ? RFS_WRITE_BLOCK : size - done;
		
		if (rfs_receive_data(&instance->sendrecv, data, current_block_size) == -1)
		{
			return -1;
		}
		
		ssize_t result = write(fd, data, current_block_size);
		
		if (result == (size_t)-1)
		{
			saved_errno = errno;
			done = (size_t)-1;
			break;
		}
		
		done += result;
	}

	struct answer ans = { cmd_write, 0, (int32_t)done, saved_errno };

	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return (done == (size_t)-1) ? 1 : 0;
}

static int _handle_mkdir(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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

static int _handle_unlink(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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

static int _handle_rmdir(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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

static int _handle_rename(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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

static int _handle_utime(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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

static int _handle_statfs(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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

static int _handle_release(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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

static int _handle_chmod(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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

static int _handle_chown(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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

static int _handle_mknod(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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

static int _handle_lock(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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

#if defined WITH_LINKS
static int _handle_symlink(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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
	
	const char *target = buffer + sizeof(len) + len;
	
	if (sizeof(len)
	+ strlen(path) + 1
	+ strlen(target) + 1 != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	errno = 0;
	int result = symlink(path, target);
	
	struct answer ans = { cmd_symlink, 0, result, errno };
	
	free_buffer(buffer);
	
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return result != 0 ? 1 : 0;
}

static int _handle_link(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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
	
	const char *target = buffer + sizeof(len) + len;
	
	if (sizeof(len)
	+ strlen(path) + 1
	+ strlen(target) + 1 != cmd->data_len)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	errno = 0;
	int result = link(path, target);
	
	struct answer ans = { cmd_link, 0, result, errno };
	
	free_buffer(buffer);
	
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return result != 0 ? 1 : 0;
}

static int _handle_readlink(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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
	
	unsigned bsize = 0;
	const char *path = buffer + 
	unpack_32(&bsize, buffer, 0);

	if (buffer[cmd->data_len-1] != 0)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}

	char *link_buffer = get_buffer(bsize);
	errno = 0;
	int ret = readlink(path, link_buffer, bsize);
	free(buffer);
	struct answer ans = { cmd_readlink, 0, ret, errno };
	if ( ret != -1 )
	{
		ans.data_len = ret+1;
		ans.ret = 0;
		link_buffer[ret] = '\0';
	}
	if (rfs_send_answer_data(&instance->sendrecv, &ans, link_buffer, ans.data_len) == -1)
	{
		free(link_buffer);
		return -1;
	}
	free(link_buffer);

	return 0;
}
#endif /* WITH_LINKS */

#ifdef WITH_SSL
static int _handle_enablessl(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	instance->sendrecv.ssl_enabled = 0;
	
	instance->sendrecv.ssl_socket = rfs_init_server_ssl(&instance->ssl.ctx, 
	instance->config.ssl_key_file, 
	instance->config.ssl_cert_file, 
	instance->config.ssl_ciphers);
	
	if (instance->sendrecv.ssl_socket == NULL)
	{
		instance->ssl.last_error = rfs_last_ssl_error(instance->ssl.last_error);
		ERROR("Error initing SSL: %s\n", instance->ssl.last_error);
		return reject_request(instance, cmd, ECANCELED) != 0 ? -1 : 1;
	}
	
	struct answer ans = { cmd_enablessl, 0, 0, 0 };
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		rfs_clear_ssl(&instance->sendrecv.ssl_socket, &instance->ssl.ctx);
		if (instance->ssl.last_error != NULL)
		{
			free(instance->ssl.last_error);
			instance->ssl.last_error = NULL;
		}
		return -1;
	}
	
	if (rfs_attach_ssl(instance->sendrecv.ssl_socket, instance->sendrecv.socket) != 0)
	{
		instance->ssl.last_error = rfs_last_ssl_error(instance->ssl.last_error);
		ERROR("SSL error: %s\n", instance->ssl.last_error);
		rfs_clear_ssl(&instance->sendrecv.ssl_socket, &instance->ssl.ctx);
		if (instance->ssl.last_error != NULL)
		{
			free(instance->ssl.last_error);
			instance->ssl.last_error = NULL;
		}
		return reject_request(instance, cmd, ECANCELED) != 0 ? -1 : 1;
	}
	
	if (rfs_accept_ssl(instance->sendrecv.ssl_socket) != 0)
	{
		instance->ssl.last_error = rfs_last_ssl_error(instance->ssl.last_error);
		DEBUG("Error accepting SSL connection: %s\n", instance->ssl.last_error);
		rfs_clear_ssl(&instance->sendrecv.ssl_socket, &instance->ssl.ctx);
		if (instance->ssl.last_error != NULL)
		{
			free(instance->ssl.last_error);
			instance->ssl.last_error = NULL;
		}
		return reject_request(instance, cmd, ECANCELED) != 0 ? -1 : 1;
	}
	
	instance->sendrecv.ssl_enabled = 1;
	
	return 0;
}
#endif /* WITH_SSL */

#if defined WITH_ACL
#include "server_handlers_acl.c"
#endif

#ifdef WITH_EXPORTS_LIST
static int _handle_listexports(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	const struct list *export_node = instance->exports.list;
	if (export_node == NULL)
	{
		return reject_request(instance, cmd, ECANCELED);
	}
	
	struct answer ans = { cmd_listexports, 0, 0, 0 };
	
	while (export_node != NULL)
	{
		const struct rfs_export *export_rec = (const struct rfs_export *)export_node->data;
		
		unsigned path_len = strlen(export_rec->path) + 1;
		uint32_t options = export_rec->options;
		
		unsigned overall_size = sizeof(options) + path_len;
		
		char *buffer = get_buffer(overall_size);
		
		pack(export_rec->path, path_len, buffer, 
		pack_32(&options, buffer, 0
		));
		
		ans.data_len = overall_size;
		
		if (rfs_send_answer_data(&instance->sendrecv, &ans, buffer, overall_size) < 0)
		{
			free_buffer(buffer);
			return -1;
		}
		
		free_buffer(buffer);
		
		export_node = export_node->next;
	}
	
	ans.data_len = 0;
	
	return (rfs_send_answer(&instance->sendrecv, &ans) < 0 ? -1 : 0);
}
#endif /* WITH_EXPORTS_LIST */

#include "server_handlers_sync.c"
