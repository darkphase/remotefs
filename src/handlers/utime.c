/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <utime.h>

#include "../buffer.h"
#include "../command.h"
#include "../config.h"
#include "../exports.h"
#include "../id_lookup.h"
#include "../instance_server.h"
#include "../sendrecv_server.h"
#include "../server.h"
#include "utils.h"

int _process_utime(struct rfsd_instance *instance, const struct command *cmd, const char *path, unsigned is_null, uint64_t actime, uint64_t modtime)
{
	struct utimbuf *buf = NULL;
	
	if (is_null == 0)
	{
		buf = malloc(sizeof(*buf));
		buf->modtime = modtime;
		buf->actime = actime;
	}
	
	errno = 0;
	int result = utime(path, buf);
	
	struct answer ans = { cmd->command, 0, result, errno };
	
	if (buf != NULL)
	{
		free(buf);
	}
		
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}
	
	return (result == 0 ? 0 : 1);
}

int _handle_utime(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = malloc(cmd->data_len);
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(&instance->sendrecv, buffer, cmd->data_len) == -1)
	{
		free(buffer);
		return -1;
	}
	
	uint16_t is_null = 0;
	uint64_t modtime = 0;
	uint64_t actime = 0;

	const char *path = 
	unpack_64(&actime, 
	unpack_64(&modtime, 
	unpack_16(&is_null, buffer
	)));
	
	if (sizeof(actime)
	+ sizeof(modtime)
	+ sizeof(is_null)
	+ strlen(path) + 1 != cmd->data_len)
	{
		free(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}

	int ret = _process_utime(instance, cmd, path, is_null, actime, modtime);

	free(buffer);

	return ret;
}
