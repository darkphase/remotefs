/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>

#include "../buffer.h"
#include "../command.h"
#include "../config.h"
#include "../defines.h"
#include "../id_lookup_client.h"
#include "../instance_client.h"
#include "../names.h"
#include "../resume/resume.h"
#include "../sendrecv_client.h"
#include "operations_rfs.h"
#include "flush.h"

uint16_t rfs_file_flags(int os_flags)
{
	uint16_t flags = 0;
	
	if (os_flags & O_APPEND)       { flags |= RFS_APPEND; }
	if (os_flags & O_ASYNC)        { flags |= RFS_ASYNC; }
	if (os_flags & O_CREAT)        { flags |= RFS_CREAT; }
	if (os_flags & O_EXCL)         { flags |= RFS_EXCL; }
	if (os_flags & O_NONBLOCK)     { flags |= RFS_NONBLOCK; }
	if (os_flags & O_NDELAY)       { flags |= RFS_NDELAY; }
	if (os_flags & O_SYNC)         { flags |= RFS_SYNC; }
	if (os_flags & O_TRUNC)        { flags |= RFS_TRUNC; }
#if defined DARWIN /* is ok for linux too */
	switch (os_flags & O_ACCMODE)
	{
		case O_RDONLY: flags |= RFS_RDONLY; break;
		case O_WRONLY: flags |= RFS_WRONLY; break;
		case O_RDWR:   flags |= RFS_RDWR;   break;
	}
#else
	if (os_flags & O_RDONLY)       { flags |= RFS_RDONLY; }
	if (os_flags & O_WRONLY)       { flags |= RFS_WRONLY; }
	if (os_flags & O_RDWR)         { flags |= RFS_RDWR; }
#endif

	return flags;
}

const char* unpack_stat(struct stat *result, const char *buffer)
{
	uint32_t mode = 0;
	uint64_t size = 0;
	uint64_t atime = 0;
	uint64_t mtime = 0;
	uint64_t ctime = 0;
	uint32_t nlink = 0;
	uint32_t blocks = 0;
	uint64_t ino = 0;

#ifdef RFS_DEBUG
	dump(buffer, STAT_BLOCK_SIZE);
#endif

	unpack_32(&blocks, 
	unpack_32(&nlink, 
	unpack_64(&ctime, 
	unpack_64(&mtime, 
	unpack_64(&atime, 
	unpack_64(&size, 
	unpack_32(&mode,
	unpack_64(&ino, buffer
	))))))));

	result->st_mode = mode;
	result->st_size = size;
	result->st_atime = atime;
	result->st_mtime = mtime;
	result->st_ctime = ctime;
	result->st_nlink = nlink;
	result->st_blocks = blocks;
	result->st_ino = ino;
	
	return buffer + STAT_BLOCK_SIZE;
}

uid_t resolve_username(struct rfs_instance *instance, const char *user)
{
	uid_t uid = instance->client.my_uid;

	if (user != NULL && instance->config.auth_user != NULL
	&& strcmp(user, instance->config.auth_user) == 0)
	{
		return uid;
	}

#ifdef WITH_UGO
	char *name = strdup(user);

#ifdef RFSNSS_AVAILABLE	
	if (instance->nss.use_nss != 0)
	{
		free(name);
		name = remote_nss_name(user, instance);
	}
#endif /* RFSNSS_AVAILABLE */
	
	if ((instance->client.export_opts & OPT_UGO) != 0)
	{
		uid = lookup_user(instance, name);
	}

	free(name);
#endif /* WITH_UGO */
	
	return uid;
}

gid_t resolve_groupname(struct rfs_instance *instance, const char *group, const char *user)
{
	gid_t gid = instance->client.my_gid;

	if (group != NULL && user != NULL 
	&& instance->config.auth_user != NULL 
	&& strcmp(user, instance->config.auth_user) == 0)
	{
		return gid;
	}

#ifdef WITH_UGO
	char *group_name = strdup(group);
	char *user_name = strdup(user);

#ifdef RFSNSS_AVAILABLE	
	if (instance->nss.use_nss != 0)
	{
		free(group_name);
		free(user_name);

		group_name = remote_nss_name(group, instance);
		user_name = remote_nss_name(user, instance);
	}
#endif /* RFSNSS_AVAILABLE */

	if ((instance->client.export_opts & OPT_UGO) != 0)
	{
		gid = lookup_group(instance, group_name, user_name);
	}

	free(group_name);
	free(user_name);
#endif /* WITH_UGO */
	
	return gid;
}


int _flush_file(struct rfs_instance *instance, const char *path)
{	
	uint64_t desc = resume_is_file_in_open_list(instance->resume.open_files, path);

	if (desc != (uint64_t)-1)
	{
		return _flush_write(instance, path, desc);
	}

	return 0;
}

int cleanup_badmsg(struct rfs_instance *instance, const struct rfs_answer *ans)
{
	DEBUG("%s\n", "cleaning bad msg");
	
	if (ans->command <= cmd_first
	|| ans->command >= cmd_last)
	{
		DEBUG("%s\n", "disconnecting");
		rfs_disconnect(instance, 0);
		return -ECONNABORTED;
	}
	
	if (rfs_ignore_incoming_data(&instance->sendrecv, ans->data_len) != ans->data_len)
	{
		DEBUG("%s\n", "disconnecting");
		rfs_disconnect(instance, 0);
		return -ECONNABORTED;
	}
	
	return -EBADMSG;
}

int check_connection(struct rfs_instance *instance)
{
	if (instance->sendrecv.connection_lost == 0)
	{
		return 0;
	}

	if(instance->sendrecv.socket != -1)
	{
		rfs_disconnect(instance, 0);
	}

#ifdef RFS_DEBUG
	if (rfs_reconnect(instance, 1, 1) == 0)
#else
	if (rfs_reconnect(instance, 0, 1) == 0)
#endif
	{
		return 0;
	}

	return -1;
}
