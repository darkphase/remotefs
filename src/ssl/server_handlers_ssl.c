/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifdef WITH_SSL

#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "server.h"
#include "../buffer.h"
#include "../command.h"
#include "../config.h"
#include "../instance_server.h"
#include "../sendrecv_server.h"
#include "../server.h"

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
		ERROR("Make sure that SSL certificate (%s) and key (%s) does exist\n", 
			instance->config.ssl_cert_file, 
			instance->config.ssl_key_file);
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
#else
int server_handlers_ssl_c_empty_module = 0;
#endif /* WITH_SSL */
