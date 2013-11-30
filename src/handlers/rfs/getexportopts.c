/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "../../command.h"
#include "../../config.h"
#include "../../exports.h"
#include "../../instance_server.h"
#include "../../sendrecv_server.h"

int _handle_getexportopts(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct rfs_command *cmd)
{
	struct rfs_answer ans = { cmd_getexportopts, 
	0,
	(instance->server.mounted_export != NULL ? instance->server.mounted_export->options : (unsigned)-1),
	(instance->server.mounted_export != NULL ? 0 : EACCES) };

	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return (instance->server.mounted_export != NULL ? 0 : 1);
}
