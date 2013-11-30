/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <arpa/inet.h>
#include <netinet/in.h>

#include "../../command.h"
#include "../../instance_server.h"

int handle_keepalive(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct rfs_command *cmd)
{
	/* no need of actual handling, 
	rfsd will update keep-alive on each operation anyway */
	return 0;
}
