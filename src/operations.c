/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

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
#include <sys/socket.h>
#include <sys/ioctl.h>

#include "config.h"
#include "operations.h"
#include "buffer.h"
#include "command.h"
#include "sendrecv.h"
#include "attr_cache.h"
#include "inet.h"
#include "keep_alive_client.h"
#include "list.h"
#include "crypt.h"
#include "id_lookup.h"
#include "path.h"
#include "sockets.h"
#include "resume.h"
#include "data_cache.h"
#include "utils.h"
#include "instance.h"
#ifdef WITH_SSL
#include "ssl.h"
#endif

/* forward declarations */
static int rfs_mount(struct rfs_instance *instance, const char *path);
static int rfs_auth(struct rfs_instance *instance, const char *user, const char *passwd);
static int rfs_request_salt(struct rfs_instance *instance);
static int rfs_keep_alive(struct rfs_instance *instance);
static int rfs_getexportopts(struct rfs_instance *instance, enum rfs_export_opts *opts);
static int rfs_setsocktimeout(struct rfs_instance *instance, const int timeout);
static int rfs_setsockbuffer(struct rfs_instance *instance, const int size);

static int _rfs_flush(struct rfs_instance *instance, const char *path, uint64_t desc);
static int _rfs_open(struct rfs_instance *instance, const char *path, int flags, uint64_t *desc);
static int _rfs_lock(struct rfs_instance *instance, const char *path, uint64_t desc, int lock_cmd, struct flock *fl);
static int _rfs_release(struct rfs_instance *instance, const char *path, uint64_t desc);

/* if connection lost after executing the operation
and operation failed, then try it one more time 

pass ret (int) as return value of this macro */
#define PARTIALY_DECORATE(ret, func, instance, args...)     \
	if(check_connection((instance)) == 0)               \
	{                                                   \
		(ret) = (func)((instance), args);           \
		if ((ret) == -ECONNABORTED                  \
		&& check_connection((instance)) == 0)       \
		{                                           \
			(ret) = (func)((instance), args);   \
		}                                           \
	}

static int check_connection(struct rfs_instance *instance)
{
	if (instance->sendrecv.connection_lost == 0)
	{
		return 0;
	}

	if(instance->sendrecv.socket != -1)
	{
		rfs_disconnect(instance, 0);
	}

#ifdef RFS_DEBUG
	if (rfs_reconnect(instance, 1, 1) == 0)
#else
	if (rfs_reconnect(instance, 0, 1) == 0)
#endif
	{
		return 0;
	}

	return -1;
}

static void* maintenance(void *void_instance)
{
	struct rfs_instance *instance = (struct rfs_instance *)(void_instance);
	
	unsigned keep_alive_slept = 0;
	unsigned attr_cache_slept = 0;
	unsigned shorter_sleep = 1; /* secs */

	while (instance->sendrecv.socket != -1
	&& instance->sendrecv.connection_lost == 0)
	{
		sleep(shorter_sleep);
		keep_alive_slept += shorter_sleep;
		attr_cache_slept += shorter_sleep;
		
		if (instance->client.maintenance_please_die != 0)
		{
			pthread_exit(0);
		}
		
		if (keep_alive_slept >= keep_alive_period()
		&& keep_alive_trylock(instance) == 0)
		{
			if (check_connection(instance) == 0)
			{
				rfs_keep_alive(instance);
			}
			
			keep_alive_unlock(instance);
			keep_alive_slept = 0;
		}
		
		if (attr_cache_slept >= ATTR_CACHE_TTL
		&& keep_alive_lock(instance) == 0)
		{
			if (cache_is_old(instance) != 0)
			{
				clear_cache(instance);
			}
			
			keep_alive_unlock(instance);
			attr_cache_slept = 0;
		}
	}

	return NULL;
}

static int resume_files(struct rfs_instance *instance)
{
	/* we're doing this inside of maintenance() call, 
	so keep alive is locked */
	
	DEBUG("%s\n", "beginning to resume connection");
	
	int ret = 0; /* last error */
	unsigned resume_failed = 0;
	
	/* reopen files */
	const struct list *open_file = instance->resume.open_files;
	while (open_file != NULL)
	{
		struct open_rec *data = (struct open_rec *)open_file->data;
		uint64_t desc = (uint64_t)-1;
		uint64_t prev_desc = data->desc;
		
		int open_ret = _rfs_open(instance, data->path, data->flags, &desc);
		if (open_ret < 0)
		{
			ret = open_ret;
			remove_file_from_open_list(instance, data->path);
			remove_file_from_locked_list(instance, data->path);
			
			/* if even single file wasn't reopened, then
			force whole operation fail to prevent descriptors
			mixing and stuff */
			
			resume_failed = 1;
			break;
		}
		
		if (desc != prev_desc)
		{
			/* nope, we're not satisfied with another descriptor 
			those descriptors will be used in read() and write() so 
			files should be opened exactly with the same descriptors */
			
			resume_failed = 1;
			break;
		}
		
		if (open_ret == 0)
		{
			const struct lock_rec *lock_info = get_lock_info(instance, data->path);
			if (lock_info != NULL)
			{
				struct flock fl = { 0 };
				fl.l_type = lock_info->type;
				fl.l_whence = lock_info->whence;
				fl.l_start = lock_info->start;
				fl.l_len = lock_info->len;
				
				int lock_ret = _rfs_lock(instance, data->path, desc, lock_info->cmd, &fl);
				
				if (lock_ret < 0)
				{
					ret = lock_ret;
					remove_file_from_locked_list(instance, data->path);
				}
			}
		}
		
		open_file = open_file->next;
	}
	
	/* if resume failed, then close all files marked as open */
	if (resume_failed != 0)
	{
		const struct list *open_file = instance->resume.open_files;
		while (open_file != NULL)
		{
			struct open_rec *data = (struct open_rec *)open_file->data;
			
			_rfs_release(instance, data->path, data->desc);
			
			/* ignore the result and keep going */
			
			remove_file_from_open_list(instance, data->path);
			remove_file_from_locked_list(instance, data->path);
			
			open_file = open_file->next;
		}
	}
	
	return ret; /* not real error. probably. but we've tried our best */
}

static int cleanup_badmsg(struct rfs_instance *instance, const struct answer *ans)
{
	DEBUG("%s\n", "cleaning bad msg");
	
	if (ans->command <= cmd_first
	|| ans->command >= cmd_last)
	{
		DEBUG("%s\n", "disconnecting");
		rfs_disconnect(instance, 0);
		return -ECONNABORTED;
	}
	
	if (rfs_ignore_incoming_data(&instance->sendrecv, ans->data_len) != ans->data_len)
	{
		DEBUG("%s\n", "disconnecting");
		rfs_disconnect(instance, 0);
		return -ECONNABORTED;
	}
	
	return -EBADMSG;
}

#ifdef WITH_SSL
int rfs_enablessl(struct rfs_instance *instance, unsigned show_errors)
{
	DEBUG("key file: %s, cert file: %s\n", instance->config.ssl_key_file, instance->config.ssl_cert_file);
	DEBUG("ciphers: %s\n", instance->config.ssl_ciphers);
	
	instance->sendrecv.ssl_socket = rfs_init_client_ssl(
	&instance->ssl.ctx, 
	instance->config.ssl_key_file, 
	instance->config.ssl_cert_file, 
	instance->config.ssl_ciphers);
	
	if (instance->sendrecv.ssl_socket == NULL)
	{
		if (show_errors != 0)
		{
			instance->ssl.last_error = rfs_last_ssl_error(instance->ssl.last_error);
			ERROR("Error initing SSL: %s\n", instance->ssl.last_error);
			if (instance->ssl.last_error != NULL)
			{
				free(instance->ssl.last_error);
				instance->ssl.last_error = NULL;
			}
		}
		return -EIO;
	}

	struct command cmd = { cmd_enablessl, 0 };
	
	if (rfs_send_cmd(&instance->sendrecv, &cmd) == -1)
	{
		if (show_errors != 0)
		{
			ERROR("Error initing SSL: %s\n", strerror(EIO));
		}
		rfs_clear_ssl(&instance->sendrecv.ssl_socket, &instance->ssl.ctx);
		return -EIO;
	}
	
	struct answer ans = { 0 };
	
	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		if (show_errors != 0)
		{
			ERROR("Error initing SSL: %s\n", strerror(EIO));
		}
		rfs_clear_ssl(&instance->sendrecv.ssl_socket, &instance->ssl.ctx);
		return -EIO;
	}
	
	if (ans.command != cmd_enablessl)
	{
		if (show_errors != 0)
		{
			ERROR("Error initing SSL: %s\n", strerror(EINVAL));
		}
		rfs_clear_ssl(&instance->sendrecv.ssl_socket, &instance->ssl.ctx);
		return -EBADMSG;
	}
	
	if (ans.ret != 0)
	{
		if (show_errors != 0)
		{
			ERROR("Error initing SSL: %s\n", strerror(ans.ret_errno));
		}
		rfs_clear_ssl(&instance->sendrecv.ssl_socket, &instance->ssl.ctx);
		return -ans.ret_errno;
	}
	
	if (rfs_attach_ssl(instance->sendrecv.ssl_socket, instance->sendrecv.socket) != 0)
	{
		if (show_errors != 0)
		{
			instance->ssl.last_error = rfs_last_ssl_error(instance->ssl.last_error);
			ERROR("SSL error: %s\n", instance->ssl.last_error);
			if (instance->ssl.last_error != NULL)
			{
				free(instance->ssl.last_error);
				instance->ssl.last_error = NULL;
			}
		}
		rfs_clear_ssl(&instance->sendrecv.ssl_socket, &instance->ssl.ctx);
		return -EIO;
	}
	
	if (rfs_connect_ssl(instance->sendrecv.ssl_socket) != 0)
	{
		if (show_errors != 0)
		{
			instance->ssl.last_error = rfs_last_ssl_error(instance->ssl.last_error);
			ERROR("Error connecting using SSL: %s\n", instance->ssl.last_error);
			if (instance->ssl.last_error != NULL)
			{
				free(instance->ssl.last_error);
				instance->ssl.last_error = NULL;
			}
		}
		rfs_clear_ssl(&instance->sendrecv.ssl_socket, &instance->ssl.ctx);
		return -EIO;
	}
	
	instance->sendrecv.ssl_enabled = 1;

	return 0;
}
#endif

#include "operations_write.c"
#include "operations_read.c"

int rfs_reconnect(struct rfs_instance *instance, unsigned int show_errors, unsigned int change_path)
{
	DEBUG("(re)connecting to %s:%d\n", instance->config.host, instance->config.server_port);
	
	int sock = rfs_connect(&instance->sendrecv, instance->config.host, instance->config.server_port);
	if (sock < 0)
	{
		if (show_errors != 0)
		{
			ERROR("Error connecting to remote host: %s\n", strerror(-sock));
		}
		return 1;
	}
	else
	{
		instance->sendrecv.socket = sock;
	}
	
#ifdef WITH_SSL
	if (instance->config.enable_ssl != 0)
	{
		int ssl_ret = rfs_enablessl(instance, show_errors);
		if (ssl_ret != 0)
		{
			#if 0
			if (show_errors != 0)
			{
				/* errors should be handled in rfs_enablessl() */
			}
			#endif
			
			rfs_clear_ssl(&instance->sendrecv.ssl_socket, &instance->ssl.ctx);
			if (instance->ssl.last_error != NULL)
			{
				free(instance->ssl.last_error);
				instance->ssl.last_error = NULL;
			}
			return ssl_ret;
		}
	}
#endif
	
	int setpid_ret = setup_soket_pid(sock, getpid());
	if (setpid_ret != 0)
	{
		if (show_errors != 0)
		{
			ERROR("Error setting socket owner: %s\n", strerror(-setpid_ret));
		}
		return -1;
	}

	if (instance->config.auth_user != NULL 
	&& instance->config.auth_passwd != NULL)
	{
		DEBUG("authenticating as %s with pwd %s\n", instance->config.auth_user, instance->config.auth_passwd);
	
		int req_ret = rfs_request_salt(instance);
		if (req_ret != 0)
		{
			if (show_errors != 0)
			{
				ERROR("Requesting salt for authentication error: %s\n", strerror(-req_ret));
			}
			rfs_disconnect(instance, 1);
			return -1;
		}
		
		int auth_ret = rfs_auth(instance, instance->config.auth_user, instance->config.auth_passwd);
		if (auth_ret != 0)
		{
			if (show_errors != 0)
			{
				ERROR("Authentication error: %s\n", strerror(-auth_ret));
			}
			rfs_disconnect(instance, 1);
			return -1;
		}
	}
	
	if (instance->config.socket_timeout != -1)
	{
		
		int server_settimeout_ret = rfs_setsocktimeout(instance, instance->config.socket_timeout);
		if (server_settimeout_ret != 0)
		{
			if (show_errors != 0)
			{
			ERROR("Error setting socket timeout on remote: %s\n", strerror(-server_settimeout_ret));
			}
			return -1;
		}
		
		int local_settimeout_ret = setup_socket_timeout(instance->sendrecv.socket, instance->config.socket_timeout);
		if (local_settimeout_ret != 0)
		{
			if (show_errors != 0)
			{
				ERROR("Error setting socket timeout: %s\n", strerror(-local_settimeout_ret));
			}
			return -1;
		}
	}
	
	if (instance->config.socket_buffer != -1)
	{
		int server_setbuf_ret = rfs_setsockbuffer(instance, instance->config.socket_buffer);
		if (server_setbuf_ret != 0)
		{
			if (show_errors != 0)
			{
				ERROR("Error setting socket buffer on remote: %s\n", strerror(-server_setbuf_ret));
			}
			return -1;
		}
		
		int local_setbuf_ret = setup_socket_buffer(instance->sendrecv.socket, instance->config.socket_buffer);
		if (local_setbuf_ret != 0)
		{
			if (show_errors != 0)
			{
				ERROR("Error setting socket buffer: %s\n", strerror(-local_setbuf_ret));
			}
			return -1;
		}
		
	}

	if (change_path != 0)
	{
		DEBUG("mounting %s\n", instance->config.path);
		
		int mount_ret = rfs_mount(instance, instance->config.path);
		if (mount_ret != 0)
		{
			if (show_errors != 0)
			{
				ERROR("Error mounting remote directory: %s\n", strerror(-mount_ret));
			}
			rfs_disconnect(instance, 1);
			return -1;
		}
		
		int getopts_ret = rfs_getexportopts(instance, &instance->client.export_opts);
		if (getopts_ret != 0)
		{
			if (show_errors != 0)
			{
				ERROR("Error getting export options from server: %s\n", strerror(-getopts_ret));
			}
			rfs_disconnect(instance, 1);
			return -1;
		}
		
		if ((instance->client.export_opts & opt_ugo) > 0)
		{
			create_uids_lookup(&instance->id_lookup.uids);
			create_gids_lookup(&instance->id_lookup.gids);
		}
		
		int resume_ret = resume_files(instance);
		if (resume_ret != 0)
		{
			/* we're not supposed to show error, since resume should happend
			only on reconnect, when rfs is in background */
			
			if (show_errors != 0) /* oh, this is odd */
			{
#ifndef RFS_DEBUG
				ERROR("Hello there!\n"
				"Normally you should not be seeing this message.\n"
				"Are you sure you are running the remotefs client you've downloaded from SourceForge? If that is the case, please notify the remotefs maintainer that you actually got this message.\n"
				"You'll find his e-mail at http://remotefs.sourceforge.net . Thank you.\n"
				"Anyway, here's the actual message:\n"
#else
				ERROR(
#endif
				"Error restoring remote files state after reconnect: %s\n",
				strerror(-resume_ret));
			}
			
			/* well, we have to count this error
			but what if file is already deleted on remote side? 
			so we'll be trapped inside of reconnect.
			i think it's better to show (some) error message later 
			than broken connection */
		}
	}

	DEBUG("%s\n", "all ok");
	return 0;
}

void* rfs_init(struct rfs_instance *instance)
{
	instance->client.my_uid = getuid();
	instance->client.my_gid = getgid();
	
	keep_alive_init(instance);
	pthread_create(&instance->client.maintenance_thread, NULL, maintenance, (void *)instance);
	
	if (instance->config.use_read_cache != 0)
	{
		init_prefetch(instance);
	}
	
	if (instance->config.use_write_cache != 0)
	{
		init_write_behind(instance);
	}

	return NULL;
}

void rfs_destroy(struct rfs_instance *instance)
{
	keep_alive_lock(instance);
	
	rfs_disconnect(instance, 1);

	if (instance->config.use_read_cache != 0)
	{
		kill_prefetch(instance);
	}
	if (instance->config.use_write_cache != 0)
	{
		kill_write_behind(instance);
	}
	
	keep_alive_unlock(instance);
	
	instance->client.maintenance_please_die = 1;

	pthread_join(instance->client.maintenance_thread, NULL);
	keep_alive_destroy(instance);
	
	destroy_cache(instance);
	destroy_resume_lists(instance);
	
#ifdef WITH_SSL
	if (instance->config.enable_ssl != 0)
	{
		rfs_clear_ssl(&instance->sendrecv.ssl_socket, &instance->ssl.ctx);
	}
#endif
	
#ifdef RFS_DEBUG
	dump_attr_stats(instance);
#endif
}

void rfs_disconnect(struct rfs_instance *instance, int gently)
{
	if (instance->sendrecv.socket == -1)
	{
		return;
	}

	if (gently != 0)
	{
		struct command cmd = { cmd_closeconnection, 0 };
		rfs_send_cmd(&instance->sendrecv, &cmd);
	}
	
	close(instance->sendrecv.socket);
	shutdown(instance->sendrecv.socket, SHUT_RDWR);
	
	instance->sendrecv.connection_lost = 1;
	instance->sendrecv.socket = -1;
	
#ifdef WITH_SSL
	if (instance->config.enable_ssl != 0)
	{
		rfs_clear_ssl(&instance->sendrecv.ssl_socket, &instance->ssl.ctx);
		instance->sendrecv.ssl_enabled = 0;
	}
#endif

#ifdef RFS_DEBUG
	dump_sendrecv_stats(&instance->sendrecv);
#endif
}

static int rfs_request_salt(struct rfs_instance *instance)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	memset(instance->client.auth_salt, 0, sizeof(instance->client.auth_salt));

	struct command cmd = { cmd_request_salt, 0 };

	if (rfs_send_cmd(&instance->sendrecv, &cmd) == -1)
	{
		return -ECONNABORTED;
	}

	struct answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}

	if (ans.command != cmd_request_salt
	|| (ans.ret == 0 && (ans.data_len < 1 || ans.data_len > sizeof(instance->client.auth_salt))))
	{
		return cleanup_badmsg(instance, &ans);
	}

	if (ans.ret == 0)
	{
		if (rfs_receive_data(&instance->sendrecv, instance->client.auth_salt, ans.data_len) == -1)
		{
			return -ECONNABORTED;
		}
	}

	return -ans.ret;
}

static int rfs_auth(struct rfs_instance *instance, const char *user, const char *passwd)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	char *crypted = passwd_hash(passwd, instance->client.auth_salt);

	memset(instance->client.auth_salt, 0, sizeof(instance->client.auth_salt));

	uint32_t crypted_len = strlen(crypted) + 1;
	unsigned user_len = strlen(user) + 1;
	unsigned overall_size = sizeof(crypted_len) + crypted_len + user_len;

	struct command cmd = { cmd_auth, overall_size };


	char *buffer = get_buffer(overall_size);

	pack(user, user_len, buffer, 
	pack(crypted, crypted_len, buffer, 
	pack_32(&crypted_len, buffer, 0
		)));

	if (rfs_send_cmd_data(&instance->sendrecv, &cmd, buffer, cmd.data_len) == -1)
	{
		free_buffer(buffer);
		free(crypted);
		return -ECONNABORTED;
	}

	free_buffer(buffer);
	free(crypted);

	struct answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}

	if (ans.command != cmd_auth
	|| ans.data_len > 0)
	{
		return cleanup_badmsg(instance, &ans);
	}

	return -ans.ret;
}

static int rfs_mount(struct rfs_instance *instance, const char *path)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path);
	struct command cmd = { cmd_changepath, path_len + 1};
	if (rfs_send_cmd_data(&instance->sendrecv, &cmd, path, cmd.data_len) == -1)
	{
		return -ECONNABORTED;
	}

	struct answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}

	if (ans.command != cmd_changepath)
	{
		return cleanup_badmsg(instance, &ans);
	}

	return ans.ret == -1 ? -ans.ret_errno : ans.ret;
}

static int rfs_getexportopts(struct rfs_instance *instance, enum rfs_export_opts *opts)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	*opts = opt_none;
	
	struct command cmd = { cmd_getexportopts, 0 };
	
	if (rfs_send_cmd(&instance->sendrecv, &cmd) == -1)
	{
		return -ECONNABORTED;
	}
	
	struct answer ans = { 0 };
	
	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}
	
	if (ans.command != cmd_getexportopts)
	{
		return cleanup_badmsg(instance, &ans);;
	}
	
	if (ans.ret >= 0)
	{
		*opts = (enum rfs_export_opts)ans.ret;
		
		DEBUG("export options: %d\n", *opts);
	}
	
	return ans.ret >= 0 ? 0 : -ans.ret_errno;
}

static int rfs_setsocktimeout(struct rfs_instance *instance, const int timeout)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}
	
	int32_t sock_timeout = (int32_t)timeout;
#define overall_size sizeof(sock_timeout)
	char buffer[overall_size] = { 0 };

	pack_32_s(&sock_timeout, buffer, 0);
	
	struct command cmd = { cmd_setsocktimeout, overall_size };
	
	if (rfs_send_cmd_data(&instance->sendrecv, &cmd, buffer, overall_size) == -1)
	{
		return -ECONNABORTED;
	}
#undef overall_size

	struct answer ans = { 0 };
	
	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}
	
	if (ans.command != cmd_setsocktimeout)
	{
		return cleanup_badmsg(instance, &ans);
	}
	
	return ans.ret != 0 ? -ans.ret_errno : ans.ret;
}

static int rfs_setsockbuffer(struct rfs_instance *instance, const int size)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}
	
	int32_t buffer_size = (int32_t)size;
#define overall_size sizeof(buffer_size)
	char buffer[overall_size] = { 0 };

	pack_32_s(&buffer_size, buffer, 0);
	
	struct command cmd = { cmd_setsockbuffer, overall_size };
	
	if (rfs_send_cmd_data(&instance->sendrecv, &cmd, buffer, overall_size) == -1)
	{
		return -ECONNABORTED;
	}
#undef overall_size
	
	struct answer ans = { 0 };
	
	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}
	
	if (ans.command != cmd_setsockbuffer)
	{
		return cleanup_badmsg(instance, &ans);;
	}
	
	return ans.ret < 0 ? -ans.ret_errno : ans.ret;
}

static int rfs_keep_alive(struct rfs_instance *instance)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	struct command cmd = { cmd_keepalive, 0 };

	if (rfs_send_cmd(&instance->sendrecv, &cmd) == -1)
	{
		return -ECONNABORTED;
	}

	return 0;
}

static int _rfs_getattr(struct rfs_instance *instance, const char *path, struct stat *stbuf)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	const struct tree_item *cached_value = get_cache(instance, path);
	if (cached_value != NULL 
	&& (time(NULL) - cached_value->time) < ATTR_CACHE_TTL )
	{
		DEBUG("%s is cached\n", path);
		memcpy(stbuf, &cached_value->data, sizeof(*stbuf));
		return 0;
	}

	unsigned path_len = strlen(path) + 1;

	struct command cmd = { cmd_getattr, path_len };
	if (rfs_send_cmd_data(&instance->sendrecv, &cmd, path, path_len) == -1)
	{
		return -ECONNABORTED;
	}

	struct answer ans = { 0 };
	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}

	if (ans.command != cmd_getattr)
	{
		return cleanup_badmsg(instance, &ans);;
	}

	if (ans.ret == -1)
	{
		return -ans.ret_errno;
	}

	char *buffer = get_buffer(ans.data_len);

	if (rfs_receive_data(&instance->sendrecv, buffer, ans.data_len) == -1)
	{
		free_buffer(buffer);
		return -ECONNABORTED;
	}

	uint32_t mode = 0;
	uint32_t user_len = 0;
	const char *user = NULL;
	uint32_t group_len = 0;
	const char *group = NULL;
	uint64_t size = 0;
	uint64_t atime = 0;
	uint64_t mtime = 0;
	uint64_t ctime = 0;

	unsigned last_pos = 
	unpack_64(&ctime, buffer, 
	unpack_64(&mtime, buffer, 
	unpack_64(&atime, buffer, 
	unpack_64(&size, buffer, 
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
		return -EBADMSG;
	}
	
	uid_t uid = (uid_t)-1;
	
	if ((instance->client.export_opts & opt_ugo) != 0
	&& strcmp(instance->config.auth_user, user) == 0)
	{
		uid = instance->client.my_uid;
		user = get_uid_name(instance->id_lookup.uids, instance->client.my_uid);
		if (user == NULL)
		{
			free_buffer(buffer);
			return -EINVAL;
		}
	}
	else
	{
		uid = lookup_user(instance->id_lookup.uids, user);
	}
	
	gid_t gid = lookup_group(instance->id_lookup.gids, group, user);
	
	DEBUG("user: %s, group: %s, uid: %d, gid: %d\n", user, group, uid, gid);

	free_buffer(buffer);

	struct stat result = { 0 };

	result.st_mode = mode;

	if ((instance->client.export_opts & opt_ugo) == 0)
	{
		result.st_uid = instance->client.my_uid;
		result.st_gid = instance->client.my_gid;
	}
	else
	{
		result.st_uid = uid;
		result.st_gid = gid;
	}
	
	result.st_size = (off_t)size;
	result.st_atime = (time_t)atime;
	result.st_mtime = (time_t)mtime;
	result.st_ctime = (time_t)ctime;

	memcpy(stbuf, &result, sizeof(*stbuf));

	if (cache_file(instance, path, &result) == NULL)
	{
		return -EIO;
	}

	return ans.ret == -1 ? -ans.ret_errno : ans.ret;
}

static int _rfs_readdir(struct rfs_instance *instance, const char *path, const rfs_readdir_callback_t callback, void *callback_data)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path) + 1;

	struct command cmd = { cmd_readdir, path_len };

	if (rfs_send_cmd_data(&instance->sendrecv, &cmd, path, path_len) == -1)
	{
		return -ECONNABORTED;
	}

	struct answer ans = { 0 };
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
	unsigned buffer_size = stat_size + sizeof(full_path);
	char *buffer = get_buffer(buffer_size);
	
	char operation_failed = 0;
	do
	{
		if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
		{
			if (buffer != NULL)
			{
				free_buffer(buffer);
			}
			
			return -ECONNABORTED;
		}
		
		if (ans.command != cmd_readdir)
		{
			if (buffer != NULL)
			{
				free_buffer(buffer);
			}
			
			return cleanup_badmsg(instance, &ans);
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
		
		if (rfs_receive_data(&instance->sendrecv, buffer, ans.data_len) == -1)
		{
			free_buffer(buffer);
			
			return -ECONNABORTED;
		}
		
		dump(buffer, ans.data_len);
		
		unsigned last_pos =
		unpack_16(&stat_failed, buffer, 
		unpack_64(&ctime, buffer, 
		unpack_64(&mtime, buffer, 
		unpack_64(&atime, buffer, 
		unpack_64(&size, buffer, 
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
			return -EBADMSG;
		}
		
		uid_t uid = (uid_t)-1;
		
		if ((instance->client.export_opts & opt_ugo) != 0
		&& strcmp(instance->config.auth_user, user) == 0)
		{
			uid = instance->client.my_uid;
			user = get_uid_name(instance->id_lookup.uids, instance->client.my_uid);
			if (user == NULL)
			{
				free_buffer(buffer);
				return -EINVAL;
			}
			}
		else
		{
			uid = lookup_user(instance->id_lookup.uids, user);
		}
		
		gid_t gid = lookup_group(instance->id_lookup.gids, group, user);
		
		DEBUG("user: %s, group: %s, uid: %d, gid: %d\n", user, group, uid, gid);
		
		if (stat_failed == 0)
		{
		int joined = path_join(full_path, sizeof(full_path), path, entry_name);
			
			if (joined < 0)
			{
				operation_failed = 1;
				return -EINVAL;
			}
			
			if (joined == 0)
			{
				stbuf.st_mode = mode;
				/* TODO: make func for this */
				if ((instance->client.export_opts & opt_ugo) == 0)
				{
					stbuf.st_uid = instance->client.my_uid;
					stbuf.st_gid = instance->client.my_gid;
				}
				else
				{
					stbuf.st_uid = uid;
					stbuf.st_gid = gid;
				}
				stbuf.st_size = (off_t)size;
				stbuf.st_atime = (time_t)atime;
				stbuf.st_mtime = (time_t)mtime;
				stbuf.st_ctime = (time_t)ctime;
			
				if (cache_file(instance, full_path, &stbuf) == NULL)
				{
					free_buffer(buffer);
					return -EIO;
				}
			}
		}
		
		if (operation_failed == 0)
		{
			if (callback(entry_name, callback_data) != 0)
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

static int _rfs_open(struct rfs_instance *instance, const char *path, int flags, uint64_t *desc)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path) + 1;
	uint16_t fi_flags = 0;
	if (flags & O_APPEND)       { fi_flags |= RFS_APPEND; }
	if (flags & O_ASYNC)        { fi_flags |= RFS_ASYNC; }
	if (flags & O_CREAT)        { fi_flags |= RFS_CREAT; }
	if (flags & O_EXCL)         { fi_flags |= RFS_EXCL; }
	if (flags & O_NONBLOCK)     { fi_flags |= RFS_NONBLOCK; }
	if (flags & O_NDELAY)       { fi_flags |= RFS_NDELAY; }
	if (flags & O_SYNC)         { fi_flags |= RFS_SYNC; }
	if (flags & O_TRUNC)        { fi_flags |= RFS_TRUNC; }
#if defined DARWIN /* is ok for linux too */
	switch (flags & O_ACCMODE)
	{
		case O_RDONLY: fi_flags |= RFS_RDONLY; break;
		case O_WRONLY: fi_flags |= RFS_WRONLY; break;
		case O_RDWR:   fi_flags |= RFS_RDWR;   break;
	}
#else
	if (flags & O_RDONLY)       { fi_flags |= RFS_RDONLY; }
	if (flags & O_WRONLY)       { fi_flags |= RFS_WRONLY; }
	if (flags & O_RDWR)         { fi_flags |= RFS_RDWR; }
#endif
	unsigned overall_size = sizeof(fi_flags) + path_len;

	struct command cmd = { cmd_open, overall_size };

	char *buffer = get_buffer(cmd.data_len);

	pack(path, path_len, buffer, 
	pack_16(&fi_flags, buffer, 0
	));

	if (rfs_send_cmd_data(&instance->sendrecv, &cmd, buffer, cmd.data_len) == -1)
	{
		free_buffer(buffer);
		return -ECONNABORTED;
	}

	free_buffer(buffer);

	struct answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}

	if (ans.command != cmd_open)
	{
		return cleanup_badmsg(instance, &ans);;
	}

	if (ans.ret != -1)
	{
		uint64_t handle = (uint64_t)-1;
		
		if (ans.data_len != sizeof(handle))
		{
			return cleanup_badmsg(instance, &ans);;
		}
		
		if (rfs_receive_data(&instance->sendrecv, &handle, ans.data_len) == -1)
		{
			return -ECONNABORTED;
		}
		
		*desc = ntohll(handle);
	}

	if (ans.ret == -1)
	{
		if (ans.ret_errno == -ENOENT)
		{
			delete_from_cache(instance, path);
		}
	}
	else
	{
		delete_from_cache(instance, path);
		add_file_to_open_list(instance, path, flags, *desc);
	}

	return ans.ret == -1 ? -ans.ret_errno : ans.ret;
}

static int _rfs_release(struct rfs_instance *instance, const char *path, uint64_t desc)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}
	
	int flush_ret = flush_write(instance, path, desc); /* make sure no data is buffered */
	if (flush_ret < 0)
	{
		return flush_ret;
	}

	clear_cache_by_desc(&instance->read_cache.cache, desc);
	clear_cache_by_desc(&instance->write_cache.cache, desc);
	
	uint64_t handle = htonll(desc);

	struct command cmd = { cmd_release, sizeof(handle) };

	if (rfs_send_cmd_data(&instance->sendrecv, &cmd, &handle, cmd.data_len) == -1)
	{
		return -ECONNABORTED;
	}

	struct answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == 0)
	{
		return -ECONNABORTED;
	}

	if (ans.command != cmd_release 
	|| ans.data_len != 0)
	{
		return cleanup_badmsg(instance, &ans);;
	}
	
	if (ans.ret == 0)
	{
		delete_from_cache(instance, path);
		remove_file_from_open_list(instance, path);
	}

	return ans.ret == -1 ? -ans.ret_errno : ans.ret;
}

static int _rfs_truncate(struct rfs_instance *instance, const char *path, off_t offset)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path) + 1;
	uint32_t foffset = offset;

	unsigned overall_size = sizeof(foffset) + path_len;

	struct command cmd = { cmd_truncate, overall_size };

	char *buffer = get_buffer(cmd.data_len);

	pack(path, path_len, buffer, 
	pack_32(&foffset, buffer, 0
	));

	if (rfs_send_cmd_data(&instance->sendrecv, &cmd, buffer, cmd.data_len) == -1)
	{
		free_buffer(buffer);
		return -ECONNABORTED;
	}

	free_buffer(buffer);

	struct answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}

	if (ans.command != cmd_truncate || ans.data_len != 0)
	{
		return cleanup_badmsg(instance, &ans);
	}

	if (ans.ret == 0)
	{
		delete_from_cache(instance, path);
	}

	return ans.ret == -1 ? -ans.ret_errno : ans.ret;
}

static int _rfs_mkdir(struct rfs_instance *instance, const char *path, mode_t mode)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path) + 1;

	uint32_t fmode = mode;

	unsigned overall_size = sizeof(fmode) + path_len;

	struct command cmd = { cmd_mkdir, overall_size };

	char *buffer = get_buffer(cmd.data_len);

	pack(path, path_len, buffer, 
	pack_32(&fmode, buffer, 0
	));

	if (rfs_send_cmd_data(&instance->sendrecv, &cmd, buffer, cmd.data_len) == -1)
	{
		free_buffer(buffer);
		return -ECONNABORTED;
	}

	free_buffer(buffer);

	struct answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}

	if (ans.command != cmd_mkdir 
	|| ans.data_len != 0)
	{
		return cleanup_badmsg(instance, &ans);;
	}

	if (ans.ret == 0)
	{
		delete_from_cache(instance, path);
	}

	return ans.ret == -1 ? -ans.ret_errno : ans.ret;
}

static int _rfs_unlink(struct rfs_instance *instance, const char *path)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path) + 1;
	struct command cmd = { cmd_unlink, path_len };

	if (rfs_send_cmd_data(&instance->sendrecv, &cmd, path, cmd.data_len) == -1)
	{
		return -ECONNABORTED;
	}

	struct answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}

	if (ans.command != cmd_unlink 
	|| ans.data_len != 0)
	{
		return cleanup_badmsg(instance, &ans);;
	}

	if (ans.ret == 0)
	{
		delete_from_cache(instance, path);
	}

	return ans.ret == -1 ? -ans.ret_errno : ans.ret;
}

static int _rfs_rmdir(struct rfs_instance *instance, const char *path)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path) + 1;
	struct command cmd = { cmd_rmdir, path_len };

	if (rfs_send_cmd_data(&instance->sendrecv, &cmd, path, cmd.data_len) == -1)
	{
		return -ECONNABORTED;
	}

	struct answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}

	if (ans.command != cmd_rmdir 
	|| ans.data_len != 0)
	{
		return cleanup_badmsg(instance, &ans);
	}

	if (ans.ret == 0)
	{
		delete_from_cache(instance, path);
	}

	return ans.ret == -1 ? -ans.ret_errno : ans.ret;
}

static int _rfs_rename(struct rfs_instance *instance, const char *path, const char *new_path)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
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

	if (rfs_send_cmd_data(&instance->sendrecv, &cmd, buffer, cmd.data_len) == -1)
	{
		free_buffer(buffer);
		return -ECONNABORTED;
	}

	free_buffer(buffer);

	struct answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}

	if (ans.command != cmd_rename 
	|| ans.data_len != 0)
	{
		return cleanup_badmsg(instance, &ans);;
	}

	if (ans.ret == 0)
	{
		delete_from_cache(instance, path);
	}

	return ans.ret == -1 ? -ans.ret_errno : ans.ret;
}

static int _rfs_utime(struct rfs_instance *instance, const char *path, struct utimbuf *buf)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path) + 1;
	uint32_t actime = 0;
	uint32_t modtime = 0;
	uint16_t is_null = (uint16_t)(buf == 0 ? (uint16_t)1 : (uint16_t)0);

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

	if (rfs_send_cmd_data(&instance->sendrecv, &cmd, buffer, cmd.data_len) == -1)
	{
		free_buffer(buffer);
		return -ECONNABORTED;
	}

	free_buffer(buffer);

	struct answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}

	if (ans.command != cmd_utime 
	|| ans.data_len != 0)
	{
		return cleanup_badmsg(instance, &ans);;
	}

	if (ans.ret==0)
	{
		delete_from_cache(instance, path);
	}
	return ans.ret == -1 ? -ans.ret_errno : ans.ret;
}

static int _rfs_statfs(struct rfs_instance *instance, const char *path, struct statvfs *buf)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path) + 1;

	struct command cmd = { cmd_statfs, path_len };

	if (rfs_send_cmd_data(&instance->sendrecv, &cmd, path, path_len) == -1)
	{
		return -ECONNABORTED;
	}

	struct answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
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

	if (ans.command != cmd_statfs 
	|| ans.data_len != overall_size)
	{
		return cleanup_badmsg(instance, &ans);;
	}

	char *buffer = get_buffer(ans.data_len);

	if (rfs_receive_data(&instance->sendrecv, buffer, ans.data_len) == -1)
	{
		free_buffer(buffer);
		return -ECONNABORTED;
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

static int _rfs_mknod(struct rfs_instance *instance, const char *path, mode_t mode, dev_t dev)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path) + 1;
	uint32_t fmode = mode;
	if ((fmode & 0777) == 0)
	{
		fmode |= 0600;
	}
	unsigned overall_size = sizeof(fmode) + path_len;

	struct command cmd = { cmd_mknod, overall_size };

	char *buffer = get_buffer(overall_size);

	pack(path, path_len, buffer, 
	pack_32(&fmode, buffer, 0
	));

	if (rfs_send_cmd_data(&instance->sendrecv, &cmd, buffer, cmd.data_len) == -1)
	{
		free_buffer(buffer);
		return -ECONNABORTED;
	}

	free_buffer(buffer);

	struct answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}

	if (ans.command != cmd_mknod)
	{
		return cleanup_badmsg(instance, &ans);
	}

	if ( ans.ret == 0 )
	{
		delete_from_cache(instance, path);
	}

	return ans.ret == 0 ? 0 : -ans.ret_errno;
}

static int _rfs_chmod(struct rfs_instance *instance, const char *path, mode_t mode)
{
	if ((instance->client.export_opts & opt_ugo) == 0)
	{
		/* actually dummy to keep some software happy. 
		do not replace with -EACCES or something */
		return 0; 
	}
	
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path) + 1;
	uint32_t fmode = mode;

	unsigned overall_size = sizeof(fmode) + path_len;

	struct command cmd = { cmd_chmod, overall_size };

	char *buffer = get_buffer(overall_size);
	pack(path, path_len, buffer, 
	pack_32(&fmode, buffer, 0
		));

	if (rfs_send_cmd_data(&instance->sendrecv, &cmd, buffer, cmd.data_len) == -1)
	{
		free_buffer(buffer);
		return -ECONNABORTED;
	}

	free_buffer(buffer);

	struct answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}

	if (ans.command != cmd_chmod)
	{
		return cleanup_badmsg(instance, &ans);;
	}

	if ( ans.ret_errno == 0 )
	{
		delete_from_cache(instance, path);
	}

	return ans.ret == 0 ? 0 : -ans.ret_errno;
}

static int _rfs_chown(struct rfs_instance *instance, const char *path, uid_t uid, gid_t gid)
{
	if ((instance->client.export_opts & opt_ugo) == 0)
	{
		/* actually dummy to keep some software happy. 
		do not replace with -EACCES or something */
		return 0;
	}
	
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path) + 1;
	
	const char *user = NULL;
	if (instance->client.my_uid == uid)
	{
		user = instance->config.auth_user;
	}
	else if ( uid == -1 )
	{
		user = "";
	}
	else
	{
		user = get_uid_name(instance->id_lookup.uids, uid);
		if (user == NULL)
		{
			return -EINVAL;
		}
	}
	
	const char *group = NULL;
	if (instance->client.my_gid == gid)
	{
		group = instance->config.auth_user; /* yes, indeed, default group to auth_user */
	}
	else if ( gid == -1 )
	{
		group = "";
	}
	else
	{
		group = get_gid_name(instance->id_lookup.gids, gid);
		if (group == NULL)
		{
			return -EINVAL;
		}
	}
	
	uint32_t user_len = strlen(user) + 1;
	uint32_t group_len = strlen(group) + 1;

	unsigned overall_size = sizeof(user_len) 
	+ sizeof(group_len) 
	+ path_len 
	+ user_len 
	+ group_len;

	struct command cmd = { cmd_chown, overall_size };

	char *buffer = get_buffer(overall_size);
	pack(group, group_len, buffer, 
	pack(user, user_len, buffer, 
	pack(path, path_len, buffer, 
	pack_32(&group_len, buffer, 
	pack_32(&user_len, buffer, 0
	)))));

	if (rfs_send_cmd_data(&instance->sendrecv, &cmd, buffer, cmd.data_len) == -1)
	{
		free_buffer(buffer);
		return -ECONNABORTED;
	}

	free_buffer(buffer);

	struct answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}

	if (ans.command != cmd_chown)
	{
		return cleanup_badmsg(instance, &ans);;
	}

	if ( ans.ret_errno == 0 )
	{
		delete_from_cache(instance, path);
	}

	return ans.ret == 0 ? 0 : -ans.ret_errno;
}

static int _rfs_lock(struct rfs_instance *instance, const char *path, uint64_t desc, int lock_cmd, struct flock *fl)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}
	
	uint16_t flags = 0;
	
	switch (lock_cmd)
	{
	case F_GETLK:
		flags = RFS_GETLK;
		break;
	case F_SETLK:
		flags = RFS_SETLK;
		break;
	case F_SETLKW:
		flags = RFS_SETLKW;
		break;
	default:
		return -EINVAL;
	}
	
	uint16_t type = 0;
	switch (fl->l_type)
	{
	case F_UNLCK:
		type = RFS_UNLCK;
		break;
	case F_RDLCK:
		type = RFS_RDLCK;
		break;
	case F_WRLCK:
		type = RFS_WRLCK;
		break;
	default:
		return -EINVAL;
	}
	
	uint16_t whence = fl->l_whence;
	uint64_t start = fl->l_start;
	uint64_t len = fl->l_len;
	uint64_t fd = desc;
	
#define overall_size sizeof(fd) + sizeof(flags) + sizeof(type) + sizeof(whence) + sizeof(start) + sizeof(len)
	char buffer[overall_size] = { 0 };
	
	pack_64(&len, buffer,
	pack_64(&start, buffer,
	pack_16(&whence, buffer, 
	pack_16(&type, buffer,
	pack_16(&flags, buffer,
	pack_64(&fd, buffer, 0
	))))));
	
	struct command cmd = { cmd_lock, overall_size };
	
	if (rfs_send_cmd_data(&instance->sendrecv, &cmd, buffer, overall_size) == -1)
	{
		return -ECONNABORTED;
	}
#undef overall_size

	struct answer ans = { 0 };
	
	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}
	
	if (ans.command != cmd_lock 
	|| ans.data_len != 0)
	{
		return cleanup_badmsg(instance, &ans);;
	}
	
	if (ans.ret == 0)
	{
		if (lock_cmd == F_SETLK)
		{
			if (fl->l_type == F_UNLCK)
			{
				remove_file_from_locked_list(instance, path);
			}
			else
			{
				add_file_to_locked_list(instance, path, lock_cmd, fl->l_type, fl->l_whence, fl->l_start, fl->l_len);
			}
		}
	}
	
	return ans.ret != 0 ? -ans.ret_errno : 0;
}

#if defined WITH_LINKS
static int _rfs_link(struct rfs_instance *instance, const char *path, const char *target)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path) + 1;
	unsigned target_len = strlen(target) + 1;
	uint32_t len = path_len;

	unsigned overall_size = sizeof(len) + path_len + target_len;

	struct command cmd = { cmd_link, overall_size };

	char *buffer = get_buffer(cmd.data_len);

	pack(target, target_len, buffer,
	pack(path, path_len, buffer,
	pack_32(&len, buffer, 0
	)));

	if (rfs_send_cmd_data(&instance->sendrecv, &cmd, buffer, cmd.data_len) == -1)
	{
		free_buffer(buffer);
		return -ECONNABORTED;
	}

	free_buffer(buffer);

	struct answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}

	if (ans.command != cmd_link 
	|| ans.data_len != 0)
	{
		return cleanup_badmsg(instance, &ans);;
	}
#if 1 /* TBD */
	if (ans.ret == 0)
	{
		delete_from_cache(instance, path);
	}
#endif 
	return ans.ret == -1 ? -ans.ret_errno : ans.ret;
}

static int _rfs_symlink(struct rfs_instance *instance, const char *path, const char *target)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path) + 1;
	unsigned target_len = strlen(target) + 1;
	uint32_t len = path_len;

	unsigned overall_size = sizeof(len) + path_len + target_len;

	struct command cmd = { cmd_symlink, overall_size };

	char *buffer = get_buffer(cmd.data_len);

	pack(target, target_len, buffer,
	pack(path, path_len, buffer,
	pack_32(&len, buffer, 0
	)));

	if (rfs_send_cmd_data(&instance->sendrecv, &cmd, buffer, cmd.data_len) == -1)
	{
		free_buffer(buffer);
		return -ECONNABORTED;
	}

	free_buffer(buffer);

	struct answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}
	
	if (ans.command != cmd_symlink 
	|| ans.data_len != 0)
	{
		return cleanup_badmsg(instance, &ans);;
	}

	if (ans.ret == 0)
	{
	delete_from_cache(instance, path);
	}

	return ans.ret == -1 ? -ans.ret_errno : ans.ret;
}

static int _rfs_readlink(struct rfs_instance *instance, const char *path, char *link_buffer, size_t size)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path) + 1;
	uint32_t bsize = size - 1; /* reserve place for ending \0 */
	int overall_size = path_len + sizeof(bsize);
	char *buffer = get_buffer(overall_size);

	pack(path, path_len, buffer,
	pack_32(&bsize, buffer, 0
	));

	struct command cmd = { cmd_readlink, overall_size };

	if (rfs_send_cmd_data(&instance->sendrecv, &cmd, buffer, cmd.data_len) == -1)
	{
		free(buffer);
		return -ECONNABORTED;
	}

	free_buffer(buffer);

	struct answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}

	if (ans.command != cmd_readlink)
	{
		return cleanup_badmsg(instance, &ans);;
	}

	/* if all was OK we will get the link info within our telegram */
	buffer = get_buffer(ans.data_len);
	memset(link_buffer, 0, ans.data_len);

	if (rfs_receive_data(&instance->sendrecv, buffer, ans.data_len) == -1)
	{
		free_buffer(buffer);
		return -ECONNABORTED;
	}
	
	if (ans.data_len >= size) /* >= to fit ending \0 into link_buffer */
	{
		return -EBADMSG;
	}

	strncpy(link_buffer, buffer, ans.data_len);
	return 0;/* ans.ret;*/ /* This is not OKm readlink shall return the size of the link */
}
#endif /* WITH_LINKS */

#if defined WITH_ACL
#include "operations_acl.c"
#endif

#ifdef WITH_EXPORTS_LIST
int rfs_list_exports(struct rfs_instance *instance)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}
	
	struct command cmd = { cmd_listexports, 0 };
	
	if (rfs_send_cmd(&instance->sendrecv, &cmd) < 0)
	{
		return -ECONNABORTED;
	}
	
	struct answer ans = { 0 };
	unsigned header_printed = 0;
	unsigned exports_count = 0;
	
	do
	{
		if (rfs_receive_answer(&instance->sendrecv, &ans) < 0)
		{
			return -ECONNABORTED;
		}
		
		if (ans.command != cmd_listexports)
		{
			return cleanup_badmsg(instance, &ans);;
		}
		
		if (ans.data_len == 0)
		{
			break;
		}
		
		if (ans.ret != 0)
		{
			return -ans.ret_errno;
		}
		
		++exports_count;
		
		char *buffer = get_buffer(ans.data_len);
		
		if (rfs_receive_data(&instance->sendrecv, buffer, ans.data_len) < 0)
		{
			free_buffer(buffer);
			return -ECONNABORTED;
		}
		
		uint32_t options = opt_none;
		
		const char *path = buffer + 
		unpack_32(&options, buffer, 0);
		
		if (header_printed == 0)
		{
			INFO("%s\n\n", "Server provides the folowing export(s):");
			header_printed = 1;
		}
		
		INFO("%s", path);
		
		if (options != opt_none)
		{
			INFO("%s", " (");
			if (((unsigned)options & opt_ro) > 0)
			{
				INFO("%s", describe_option(opt_ro));
			}
			else if (((unsigned)options & opt_ugo) > 0)
			{
				INFO("%s", describe_option(opt_ugo));
			}
			INFO("%s\n", ")");
		}
		else
		{
		INFO("%s\n", "");
		}
		
		free_buffer(buffer);
	}
	while (ans.data_len != 0
	&& ans.ret == 0
	&& ans.ret_errno == 0);
	
	if (exports_count == 0)
	{
		INFO("%s\n", "Server provides no exports (this is odd)");
	}
	else
	{
		INFO("%s\n", "");
	}
	
	return 0;
}
#endif /* WITH_EXPORTS_LIST */

#include "operations_sync.c"

#undef PARTIALY_DECORATE
