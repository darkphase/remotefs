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
#include "../handling.h"
#include "../id_lookup.h"
#include "../instance_server.h"
#include "../sendrecv_server.h"
#include "utils.h"
#include "utime.h"

int _handle_utimens(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct rfs_command *cmd)
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
	uint64_t modtime_sec = 0;
	uint64_t modtime_nsec = 0;
	uint64_t actime_sec = 0;
	uint64_t actime_nsec = 0;

	const char *path = 
	unpack_16(&is_null, 
	unpack_64(&actime_nsec, 
	unpack_64(&actime_sec, 
	unpack_64(&modtime_nsec, 
	unpack_64(&modtime_sec, buffer
	)))));

	if (sizeof(actime_sec)
	+ sizeof(actime_nsec)
	+ sizeof(modtime_sec)
	+ sizeof(modtime_nsec)
	+ sizeof(is_null)
	+ strlen(path) + 1 != cmd->data_len)
	{
		free(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	int ret = _process_utime(instance, cmd, path, is_null, actime_sec, modtime_sec);

	free(buffer);
	
	return ret;
}
