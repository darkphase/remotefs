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
#include "../../buffer.h"
#include "../../changelog.h"
#include "../../command.h"
#include "../../config.h"
#include "../../instance_client.h"
#include "../../sendrecv_client.h"
#include "../../version.h"
#include "../utils.h"
#include "handshake.h"

int rfs_handshake(struct rfs_instance *instance)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	uint16_t major = RFS_VERSION_MAJOR;
	uint16_t minor = RFS_VERSION_MINOR;

#define overall_size sizeof(major) + sizeof(minor)
	char buffer[overall_size] = { 0 };

	pack_16(&minor,
	pack_16(&major, buffer));

	struct rfs_command cmd = { cmd_handshake, overall_size };

	send_token_t token = { 0 };
	if (do_send(&instance->sendrecv,
		queue_data(buffer, cmd.data_len,
		queue_cmd(&cmd, &token))) < 0)
	{
		return -ECONNABORTED;
	}

	struct rfs_answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}

	if (ans.command != cmd_handshake
	|| ans.data_len != overall_size)
	{
		return cleanup_badmsg(instance, &ans);
	}

	if (ans.ret != 0)
	{
		return -ans.ret_errno;
	}

	if (rfs_receive_data(&instance->sendrecv, buffer, overall_size) == -1)
	{
		return -ECONNABORTED;
	}
#undef overall_size

	uint16_t server_major = 0;
	uint16_t server_minor = 0;

	unpack_16(&server_minor,
	unpack_16(&server_major, buffer));

	DEBUG("server reported version %d.%d\n", server_major, server_minor);

	int compatible = my_version_compatible(COMPAT_VERSION(server_major, server_minor));

	if (compatible == 0)
	{
		return -EPROTONOSUPPORT;
	}

	return 0;
}
