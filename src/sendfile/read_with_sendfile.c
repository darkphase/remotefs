/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <sys/stat.h>

#include "../options.h"
#include "../buffer.h"
#include "../command.h"
#include "../instance_server.h"
#include "../sendrecv_server.h"
#include "../handling.h"

#include "sendfile_rfs.h"

#ifdef SENDFILE_AVAILABLE
int read_with_sendfile(struct rfsd_instance *instance, const struct command *cmd, uint64_t handle, off_t offset, size_t size)
{
	DEBUG("%s\n", "reading with sendfile");
	
	int fd = (int)handle;

	struct stat st = { 0 };
	errno = 0;
	if (fstat(fd, &st) != 0)
	{
		return reject_request(instance, cmd, errno) == 0 ? 1 : -1;
	}
	
	if (offset > st.st_size)
	{
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}

	struct answer ans = { cmd_read, (uint32_t)size, (int32_t)size, 0};
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}

	size_t read_size = size;
	if (read_size + offset > st.st_size)
	{
		read_size = st.st_size - offset;
	}
	
	errno = 0;
	size_t done = 0;
	while (done < read_size)
	{	
		ssize_t result = rfs_sendfile(instance->sendrecv.socket, fd, offset + done, read_size - done);
		
		if (result <= 0)
		{
			ans.command = cmd_read;
			ans.ret_errno = errno;
			ans.ret = -1;
			ans.data_len = 0;

			return rfs_send_answer_oob(&instance->sendrecv, &ans) == -1 ? -1 : 1;
		}
		
		done += result;
#ifdef RFS_DEBUG
		if (result > 0)
		{
			instance->sendrecv.bytes_sent += result;
		}
#endif
	}

	if (read_size != size)
	{
		ans.command = cmd_read;
		ans.ret_errno = errno;
		ans.ret = (int32_t)read_size;
		ans.data_len = 0;
		return rfs_send_answer_oob(&instance->sendrecv, &ans) == -1 ? -1 : 1;
	}
	
	return 0;
}

#endif /* SENDFILE_AVAILABLE */

