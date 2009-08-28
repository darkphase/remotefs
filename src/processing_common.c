/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "config.h"
#include "nss_cmd.h"
#include "processing_common.h"

int send_answer(int sock, const struct nss_answer *ans, const char *data)
{	
	DEBUG("sending answer: %s\n", describe_nss_command(ans->command));
	if (send(sock, ans, sizeof(*ans), 0) != sizeof(*ans))
	{
		return -1;
	}

	if (data != NULL 
	&& send(sock, data, ans->data_len, 0) != ans->data_len)
	{
		DEBUG("sending %d bytes of data\n", ans->data_len);
		return -1;
	}

	return 0;
}

int recv_command_data(int sock, const struct nss_command *cmd, char **data)
{
	if (cmd->data_len == 0)
	{
		return -EINVAL;
	}

	char *buffer = malloc(cmd->data_len);
	
	ssize_t done = recv(sock, buffer, cmd->data_len, 0);

	if (done == cmd->data_len)
	{
		*data = buffer;
		return 0;
	}
	else
	{
		free(buffer);
		*data = NULL;
		return -1;
	}
}

int reject_request(int sock, const struct nss_command *cmd, int ret_errno)
{
	DEBUG("%s\n", "rejecting request");

	struct nss_answer ans = { cmd->command, 0, -1, ret_errno };
	return send_answer(sock, &ans, NULL);
}

