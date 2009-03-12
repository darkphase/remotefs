/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "buffer.h"
#include "command.h"
#include "config.h"
#include "crypt.h"
#include "exports.h"
#include "id_lookup.h"
#include "instance_server.h"
#include "list.h"
#include "passwd.h"
#include "sockets.h"
#include "utils.h"
#include "server.h"
#include "sendrecv.h"
#ifdef WITH_SSL
#	include "ssl_server.h"
#endif

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
		if (
#ifdef WITH_UGO
		(export_info->options & OPT_UGO) == 0 &&
#endif
		is_ipaddr(user) != 0)
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

int _handle_request_salt(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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

int _handle_auth(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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

int _handle_closeconnection(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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

#ifdef WITH_UGO
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
#endif

static int setup_export_opts(struct rfsd_instance *instance, const struct rfs_export *export_info, uid_t user_uid, gid_t user_gid)
{
	/* always set gid first :)
	*/
	
#ifdef WITH_UGO
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
#endif
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

int _handle_changepath(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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

	uid_t user_uid = geteuid();
	gid_t user_gid = getegid();

#ifdef WITH_UGO
	/* if we're going into UGO, then user_uid and user_gid should 
	point to logged user */

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

		int init_errno = init_groups_for_ugo(instance, user_gid);
		if (init_errno != 0)
		{
			free_buffer(buffer);
			return reject_request(instance, cmd, -init_errno) == 0 ? 1 : -1;
		}
		
		create_uids_lookup(&instance->id_lookup.uids);
		create_gids_lookup(&instance->id_lookup.gids);
	}
#endif

	errno = 0;
	int result = chroot(path);

	struct answer ans = { cmd_changepath, 0, result, errno };
	
	free_buffer(buffer);
	
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

int _handle_getexportopts(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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

#ifdef WITH_SSL
int _handle_enablessl(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
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

