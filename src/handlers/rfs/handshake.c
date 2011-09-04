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

#include "../../auth.h"
#include "../../changelog.h"
#include "../../command.h"
#include "../../config.h"
#include "../../handling.h"
#include "../../instance_server.h"
#include "../../sendrecv_server.h"
#include "../../version.h"

int _handle_handshake(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	uint16_t major = 0;
	uint16_t minor = 0;

#define overall_size sizeof(major) + sizeof(minor)
	if (cmd->data_len != overall_size)
	{
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}

	char buffer[overall_size] = { 0 };

	if (rfs_receive_data(&instance->sendrecv, buffer, cmd->data_len) == -1)
	{
		return -1;
	}

	unpack_16(&minor,
	unpack_16(&major, buffer));

	DEBUG("client reported version %u.%u\n", major, minor);

	int compatible = my_version_compatible(COMPAT_VERSION(major, minor));

	uint16_t my_major = RFS_VERSION_MAJOR;
	uint16_t my_minor = RFS_VERSION_MINOR;

	pack_16(&my_minor,
	pack_16(&my_major, buffer));

	struct answer ans = { cmd_handshake, overall_size, (compatible != 0 ? 0 : -1), (compatible != 0 ? 0 : EINVAL) };
#undef overall_size

	if (ans.ret == 0)
	{
		instance->server.hand_shaken = 1;
	}

	send_token_t token = { 0 };

	return do_send(&instance->sendrecv,
		queue_data(buffer, ans.data_len,
		queue_ans(&ans, &token
		))) < 0 ? -1 : 0;
}
