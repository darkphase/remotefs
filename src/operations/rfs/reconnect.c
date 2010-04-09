/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../../options.h"
#include "../../config.h"
#include "../../connect.h"
#include "../../instance_client.h"
#include "../../resume/client.h"
#include "../../sockets.h"
#include "../operations_rfs.h"

int rfs_reconnect(struct rfs_instance *instance, unsigned int show_errors, unsigned int change_path)
{
	DEBUG("(re)connecting to %s:%d\n", instance->config.host, instance->config.server_port);
	
	int sock = rfs_connect(&instance->sendrecv, instance->config.host, instance->config.server_port,
#if defined WITH_IPV6
	                       instance->config.force_ipv4, instance->config.force_ipv6);
#else
	                       0, 0);
#endif
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