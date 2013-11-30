/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../../command.h"
#include "../../config.h"
#include "../../instance_server.h"
#include "../../server.h"
#include "../../sendrecv_server.h"

int _handle_closeconnection(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct rfs_command *cmd)
{
#ifdef RFS_DEBUG
#ifndef WITH_IPV6
	DEBUG("connection to %s is closed\n", inet_ntoa(client_addr->sin_addr));
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
	DEBUG("connection to %s is closed\n", straddr);
#endif /* WITH_IPV6 */
#endif
	release_rfsd_instance(instance);
	server_close_connection(instance);
	exit(0);
}
