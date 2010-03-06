/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifdef WITH_SSL

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "../../buffer.h"
#include "../../command.h"
#include "../../config.h"
#include "../../instance_client.h"
#include "../../sendrecv_client.h"
#include "../client.h"

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
			ERROR("Make sure that SSL certificate (%s) and key (%s) do exist and correct\n", 
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
#else 
int operations_ssl_enablessl_c_empty_module = 0;
#endif /* WITH_SSL */
