/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <string.h>

#include "buffer.h"
#include "command.h"
#include "config.h"
#include "instance_client.h"
#include "keep_alive_client.h"
#include "list.h"
#include "operations.h"
#include "operations_rfs.h"
#include "operations_write.h"
#include "resume.h"
#include "sendrecv_client.h"

static int _read(struct rfs_instance *instance, char *buf, size_t size, off_t offset, uint64_t desc)
{
	DEBUG("*** reading %lu bytes at %llu\n", (unsigned long)size, (unsigned long long)offset);
	
	instance->sendrecv.oob_received = 0;
	
	uint64_t handle = desc;
	uint64_t foffset = offset;
	uint32_t fsize = size;

#define overall_size sizeof(fsize) + sizeof(foffset) + sizeof(handle)
	struct command cmd = { cmd_read, overall_size };

	char buffer[overall_size] = { 0 };

	pack_64(&handle, buffer, 
	pack_64(&foffset, buffer, 
	pack_32(&fsize, buffer, 0
	)));

	MAKE_SEND_TOK(2) token = { 2, {{ 0 }} };
	token.iov[0].iov_base = (void *)hton_cmd(&cmd);
	token.iov[0].iov_len = sizeof(cmd);
	token.iov[1].iov_base = buffer;
	token.iov[1].iov_len = overall_size;
#undef  overall_size

	if (do_send(&instance->sendrecv, (send_tok *)&token) < 0)
	{
		return -ECONNABORTED;
	}

	struct answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}

	if (ans.command != cmd_read 
	|| ans.data_len > size)
	{
		rfs_ignore_incoming_data(&instance->sendrecv, ans.data_len);
		return -EBADMSG;
	}

	if (ans.ret < 0)
	{
		return -ans.ret_errno;
	}

	if (ans.data_len > 0)
	{
		if (rfs_receive_data_oob(&instance->sendrecv, buf, ans.data_len) == -1
		&& instance->sendrecv.oob_received == 0)
		{
			return -ECONNABORTED;
		}
	}
	
	if (instance->sendrecv.oob_received != 0)
	{
		DEBUG("%s\n", "something is wrong, reading fixed answer");

		instance->sendrecv.oob_received = 0;

		if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
		{
			return -ECONNABORTED;
		}
		
		if (ans.command != cmd_read)
		{
			return -EBADMSG;
		}
		
		if (ans.ret < 0)
		{
			return -ans.ret_errno;
		}
		
		return ans.ret;
	}

	return ans.data_len;
}

int _rfs_read(struct rfs_instance *instance, const char *path, char *buf, size_t size, off_t offset, uint64_t desc)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	if (instance->config.use_write_cache != 0)
	{
		flush_write(instance, path, desc);
	}

	return _read(instance, buf, size, offset, desc);
}

