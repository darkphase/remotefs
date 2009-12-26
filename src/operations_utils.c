/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>

#include "buffer.h"
#include "config.h"
#include "defines.h"
#include "id_lookup_client.h"
#include "instance_client.h"
#include "names.h"
#include "operations_utils.h"

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

#ifdef RFS_DEBUG
	dump(buffer, STAT_BLOCK_SIZE);
#endif

	unpack_32(&blocks, 
	unpack_32(&nlink, 
	unpack_64(&ctime, 
	unpack_64(&mtime, 
	unpack_64(&atime, 
	unpack_64(&size, 
	unpack_32(&mode, buffer
	)))))));

	result->st_mode = mode;
	result->st_size = size;
	result->st_atime = atime;
	result->st_mtime = mtime;
	result->st_ctime = ctime;
	result->st_nlink = nlink;
	result->st_blocks = blocks;
	
	return buffer + STAT_BLOCK_SIZE;
}

uid_t resolve_username(struct rfs_instance *instance, const char *user)
{
#ifdef WITH_UGO
#ifdef RFSNSS_AVAILABLE
	if (instance->nss.use_nss != 0)
	{
		char *long_name = remote_nss_name(user, instance);
		
		DEBUG("long user name: %s\n", long_name);

		struct passwd *pwd = getpwnam(long_name);
		free_buffer(long_name);

		if (pwd != NULL)
		{
			DEBUG("uid for long name: %u\n", pwd->pw_uid);
			return pwd->pw_uid;
		}
	}
	else
#endif /* RFSNSS_AVAILABLE */
	if ((instance->client.export_opts & OPT_UGO) != 0)
	{
		return lookup_user(user);
	}
#endif /* WITH_UGO */
	
	return instance->client.my_uid;
}

gid_t resolve_groupname(struct rfs_instance *instance, const char *group, const char *user)
{
#ifdef WITH_UGO
#ifdef RFSNSS_AVAILABLE	
	if (instance->nss.use_nss != 0)
	{
		char *long_name = remote_nss_name(user, instance);
		
		DEBUG("long group name: %s\n", long_name);

		struct group *grp = getgrnam(long_name);
		free_buffer(long_name);

		if (grp != NULL)
		{
			DEBUG("gid for long name: %u\n", grp->gr_gid);
			return grp->gr_gid;
		}
	}
	else
#endif /* RFSNSS_AVAILABLE */
	if ((instance->client.export_opts & OPT_UGO) != 0)
	{
		return lookup_group(group, user);
	}
#endif /* WITH_UGO */
	
	return instance->client.my_gid;
}

