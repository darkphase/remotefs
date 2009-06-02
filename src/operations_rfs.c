/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#ifdef SOLARIS
#	include <fcntl.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <unistd.h>

#include "attr_cache.h"
#include "buffer.h"
#include "command.h"
#include "config.h"
#include "crypt.h"
#include "id_lookup.h"
#include "instance_client.h"
#include "keep_alive_client.h"
#include "list.h"
#ifdef WITH_UGO
#	include "nss_server.h"
#endif
#include "operations.h"
#include "operations_rfs.h"
#include "resume.h"
#ifdef WITH_SCHEDULING
#	include "scheduling.h"
#endif
#include "sendrecv.h"
#include "sockets.h"
#ifdef WITH_SSL
#	include "ssl_client.h"
#endif

int cleanup_badmsg(struct rfs_instance *instance, const struct answer *ans)
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

int check_connection(struct rfs_instance *instance)
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
		
		DEBUG("reopening file %s\n", data->path);

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

		DEBUG("%s\n", "ok");
		
		open_file = open_file->next;
	}
		
	if (resume_failed == 0)
	{
		const struct list *lock_item = instance->resume.locked_files;
		while (lock_item != NULL)
		{
			const struct lock_rec *lock_info = (const struct lock_rec *)lock_item->data;
				
			DEBUG("relocking file %s (at %ld of len %ld)\n", 
				lock_info->path, 
				lock_info->start, 
				lock_info->len);

			struct flock fl = { 0 };
			fl.l_type = lock_info->type;
			fl.l_whence = lock_info->whence;
			fl.l_start = lock_info->start;
			fl.l_len = lock_info->len;

			uint64_t desc = is_file_in_open_list(instance, lock_info->path);
			if (desc == (uint64_t)-1) /* we can only resume files which were opened on resume 
			in other case, we don't know which open flags were used to lock that file and etc
			so we can't reopen file on our own */
			{
				ret = -EBADF;
				resume_failed = 1;

				break;
			}

			int lock_ret = _rfs_lock(instance, lock_info->path, desc, lock_info->cmd, &fl);
				
			if (lock_ret < 0)
			{
				ret = lock_ret;
				remove_file_from_locked_list(instance, lock_info->path);

				resume_failed = 1;
				break;
			}
		
			DEBUG("%s\n", "ok");

			lock_item = lock_item->next;
		}
	}
		
	/* if resume failed, then close all files marked as open 
	and clear unlock locked files*/
	if (resume_failed != 0)
	{
		DEBUG("%s\n", "resume failed");

		const struct list *locked_file = instance->resume.locked_files;
		while (locked_file != NULL)
		{
			struct lock_rec *data = (struct lock_rec *)locked_file->data;
			
			struct flock fl = { 0 };
			fl.l_type = data->type;
			fl.l_whence = data->whence;
			fl.l_start = data->start;
			fl.l_len = data->len;

			uint64_t desc = is_file_in_open_list(instance, data->path);

			if (desc != (uint64_t)-1)
			{
				_rfs_lock(instance, data->path, desc, F_UNLCK, &fl); /* ignore the result and keep going */
			}
				
			remove_file_from_locked_list(instance, data->path);
			
			locked_file = locked_file->next;
		}

		const struct list *open_file = instance->resume.open_files;
		while (open_file != NULL)
		{
			struct open_rec *data = (struct open_rec *)open_file->data;
			
			_rfs_release(instance, data->path, data->desc); /* ignore the result and keep going */

			remove_file_from_open_list(instance, data->path);
			
			open_file = open_file->next;
		}
	}
	
	return ret; /* not real error. probably. but we've tried our best */
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
			ERROR("Make sure that SSL certificate (%s) and key (%s) do exist\n", 
				instance->config.ssl_cert_file, 
				instance->config.ssl_key_file);
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
	
	int setpid_ret = setup_soket_pid(sock, instance->client.my_pid);
	if (setpid_ret != 0)
	{
		if (show_errors != 0)
		{
			ERROR("Error setting socket owner: %s\n", strerror(-setpid_ret));
		}
		return -1;
	}

	setup_socket_ndelay(sock, 1);

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
		
		int resume_ret = resume_files(instance);
		if (resume_ret != 0)
		{
			/* we're not supposed to show error, since resume should happend
			only on reconnect when rfs is in background */
			
			if (show_errors != 0) /* oh, this is odd */
			{
				ERROR(
#ifndef RFS_DEBUG
				"Hello there!\n"
				"Normally you should not be seeing this message.\n"
				"Are you sure you are running the remotefs client you've downloaded from SourceForge? If that is the case, please notify the remotefs maintainer that you actually got this message.\n"
				"You'll find his e-mail at http://remotefs.sourceforge.net . Thank you.\n"
				"Anyway, here's the actual message:\n"
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

	return 0;
}

#ifdef WITH_UGO
static int init_nss_server(struct rfs_instance *instance, unsigned show_errors)
{
	if ((instance->client.export_opts & OPT_UGO) != 0)
	{
		if (is_nss_running(instance) == 0)
		{
			int getnames_ret = rfs_getnames(instance);
			if (getnames_ret != 0)
			{
				if (show_errors != 0)
				{
					ERROR("Error getting NSS lists from server: %s\n", strerror(-getnames_ret));
				}
				return -1;
			}

			int nss_start_ret = start_nss_server(instance);
			DEBUG("nss start ret: %d\n", nss_start_ret);
			if (nss_start_ret != 0)
			{
				if (show_errors != 0)
				{
					WARN("Error starting NSS server: %s\n", strerror(-nss_start_ret));
				}
				return -1;
			}
		}		
	}

	return 0;
}
#endif

void* rfs_init(struct rfs_instance *instance)
{
	keep_alive_init(instance);
	if (pthread_create(&instance->client.maintenance_thread, NULL, maintenance, (void *)instance) != 0)
	{
		instance->client.maintenance_thread = 0;
		/* TODO: how to handle ? */
	}
	
	if (instance->config.use_write_cache != 0)
	{
		init_write_behind(instance);
	}

#ifdef WITH_UGO
	if ((instance->client.export_opts & OPT_UGO) != 1)
	{
		create_uids_lookup(&instance->id_lookup.uids);
		create_gids_lookup(&instance->id_lookup.gids);
	}

	if (init_nss_server(instance, 0) != 0)
	{
		instance->nss.use_nss = 0;
	}
#endif

#ifdef WITH_SCHEDULING
	set_scheduler();
#endif

	return NULL;
}

void rfs_destroy(struct rfs_instance *instance)
{
	keep_alive_lock(instance);

#ifdef WITH_UGO
	if (is_nss_running(instance))
	{
		stop_nss_server(instance);
	}
#endif
	
	rfs_disconnect(instance, 1);

	if (instance->config.use_write_cache != 0)
	{
		kill_write_behind(instance);
	}
	
	keep_alive_unlock(instance);
	
	instance->client.maintenance_please_die = 1;

	if (instance->client.maintenance_thread != 0)
	{
		pthread_join(instance->client.maintenance_thread, NULL);
	}

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
	
	shutdown(instance->sendrecv.socket, SHUT_RDWR);
	close(instance->sendrecv.socket);
	
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

int rfs_request_salt(struct rfs_instance *instance)
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

int rfs_auth(struct rfs_instance *instance, const char *user, const char *passwd)
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

	if (rfs_send_cmd_data(&instance->sendrecv, &cmd, buffer) == -1)
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

int rfs_mount(struct rfs_instance *instance, const char *path)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path);
	struct command cmd = { cmd_changepath, path_len + 1};
	if (rfs_send_cmd_data(&instance->sendrecv, &cmd, path) == -1)
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

int rfs_getexportopts(struct rfs_instance *instance, enum rfs_export_opts *opts)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	*opts = OPT_NONE;
	
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

int rfs_keep_alive(struct rfs_instance *instance)
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

