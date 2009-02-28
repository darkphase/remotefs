/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <utime.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include <grp.h>

#include "config.h"
#include "resume.h"
#include "operations.h"
#include "operations_rfs.h"
#include "buffer.h"
#include "command.h"
#include "sendrecv.h"
#include "attr_cache.h"
#include "inet.h"
#include "list.h"
#include "path.h"
#include "sockets.h"
#include "data_cache.h"
#include "utils.h"
#include "instance_client.h"

#if defined WITH_LINKS
int _rfs_link(struct rfs_instance *instance, const char *path, const char *target)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path) + 1;
	unsigned target_len = strlen(target) + 1;
	uint32_t len = path_len;

	unsigned overall_size = sizeof(len) + path_len + target_len;

	struct command cmd = { cmd_link, overall_size };

	char *buffer = get_buffer(cmd.data_len);

	pack(target, target_len, buffer,
	pack(path, path_len, buffer,
	pack_32(&len, buffer, 0
	)));

	if (rfs_send_cmd_data(&instance->sendrecv, &cmd, buffer, cmd.data_len) == -1)
	{
		free_buffer(buffer);
		return -ECONNABORTED;
	}

	free_buffer(buffer);

	struct answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}

	if (ans.command != cmd_link 
	|| ans.data_len != 0)
	{
		return cleanup_badmsg(instance, &ans);;
	}
	
	if (ans.ret == 0)
	{
		delete_from_cache(instance, path);
	}

	return ans.ret == -1 ? -ans.ret_errno : ans.ret;
}

int _rfs_symlink(struct rfs_instance *instance, const char *path, const char *target)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path) + 1;
	unsigned target_len = strlen(target) + 1;
	uint32_t len = path_len;

	unsigned overall_size = sizeof(len) + path_len + target_len;

	struct command cmd = { cmd_symlink, overall_size };

	char *buffer = get_buffer(cmd.data_len);

	pack(target, target_len, buffer,
	pack(path, path_len, buffer,
	pack_32(&len, buffer, 0
	)));

	if (rfs_send_cmd_data(&instance->sendrecv, &cmd, buffer, cmd.data_len) == -1)
	{
		free_buffer(buffer);
		return -ECONNABORTED;
	}

	free_buffer(buffer);

	struct answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}
	
	if (ans.command != cmd_symlink 
	|| ans.data_len != 0)
	{
		return cleanup_badmsg(instance, &ans);;
	}

	if (ans.ret == 0)
	{
	delete_from_cache(instance, path);
	}

	return ans.ret == -1 ? -ans.ret_errno : ans.ret;
}

int _rfs_readlink(struct rfs_instance *instance, const char *path, char *link_buffer, size_t size)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path) + 1;
	uint32_t bsize = size - 1; /* reserve place for ending \0 */
	int overall_size = path_len + sizeof(bsize);
	char *buffer = get_buffer(overall_size);

	pack(path, path_len, buffer,
	pack_32(&bsize, buffer, 0
	));

	struct command cmd = { cmd_readlink, overall_size };

	if (rfs_send_cmd_data(&instance->sendrecv, &cmd, buffer, cmd.data_len) == -1)
	{
		free(buffer);
		return -ECONNABORTED;
	}

	free_buffer(buffer);

	struct answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}

	if (ans.command != cmd_readlink)
	{
		return cleanup_badmsg(instance, &ans);;
	}

	/* if all was OK we will get the link info within our telegram */
	buffer = get_buffer(ans.data_len);
	memset(link_buffer, 0, ans.data_len);

	if (rfs_receive_data(&instance->sendrecv, buffer, ans.data_len) == -1)
	{
		free_buffer(buffer);
		return -ECONNABORTED;
	}
	
	if (ans.data_len >= size) /* >= to fit ending \0 into link_buffer */
	{
		return -EBADMSG;
	}

	strncpy(link_buffer, buffer, ans.data_len);
	return 0;/* ans.ret;*/ /* This is not OKm readlink shall return the size of the link */
}
#else
int operations_links_c_empty_module_makes_suncc_sad = 0;
#endif /* WITH_LINKS */

