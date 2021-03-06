/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../../options.h"
#include "../../command.h"
#include "../../config.h"
#include "../../instance_client.h"
#include "../../sendrecv_client.h"

void rfs_disconnect(struct rfs_instance *instance, int gently)
{
	if (instance->sendrecv.socket == -1)
	{
		return;
	}

	if (gently != 0)
	{
		struct rfs_command cmd = { cmd_closeconnection, 0 };
		rfs_send_cmd(&instance->sendrecv, &cmd);
	}
	
	shutdown(instance->sendrecv.socket, SHUT_RDWR);
	close(instance->sendrecv.socket);
	
	instance->sendrecv.connection_lost = 1;
	instance->sendrecv.socket = -1;
	
#ifdef RFS_DEBUG
	dump_sendrecv_stats(&instance->sendrecv);
#endif
}
