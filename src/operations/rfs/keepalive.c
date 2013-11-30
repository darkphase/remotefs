/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>

#include "../../options.h"
#include "../../command.h"
#include "../../config.h"
#include "../../instance_client.h"
#include "../../sendrecv_client.h"

int rfs_keep_alive(struct rfs_instance *instance)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	struct rfs_command cmd = { cmd_keepalive, 0 };

	if (rfs_send_cmd(&instance->sendrecv, &cmd) == -1)
	{
		return -ECONNABORTED;
	}

	return 0;
}
