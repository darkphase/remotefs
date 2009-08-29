/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

#include "buffer.h"
#include "config.h"
#include "exports.h"
#include "id_lookup.h"
#include "instance_server.h"
#include "server_handlers_utils.h"

int stat_file(struct rfsd_instance *instance, const char *path, struct stat *stbuf)
{
	errno = 0;

	if (lstat(path, stbuf) != 0)
	{
		return errno;
	}
	
	if (instance->server.mounted_export != NULL
	&& (instance->server.mounted_export->options & OPT_RO) != 0)
	{
		stbuf->st_mode &= (~S_IWUSR);
		stbuf->st_mode &= (~S_IWGRP);
		stbuf->st_mode &= (~S_IWOTH);
	}
	
	return 0;
}

size_t stat_size(struct rfsd_instance *instance, struct stat *stbuf, int *ret)
{
	const char *user = get_uid_name(instance->id_lookup.uids, stbuf->st_uid);
	const char *group = get_gid_name(instance->id_lookup.gids, stbuf->st_gid);
	
#ifdef WITH_UGO
	if ((instance->server.mounted_export->options & OPT_UGO) != 0 
	&& (user == NULL
	|| group == NULL))
	{
		*ret = ECANCELED;
	}
#endif
	
	if (user == NULL)
	{
		user = "";
	}
	if (group == NULL)
	{
		group = "";
	}
	
	uint32_t user_len = strlen(user) + 1;
	uint32_t group_len = strlen(group) + 1;

	return 0
	+ sizeof(uint32_t) /* mode */
	+ sizeof(uint32_t) /* user_len */ 
	+ sizeof(uint32_t) /* group_len */
	+ sizeof(uint64_t) /* size */
	+ sizeof(uint64_t) /* atime */
	+ sizeof(uint64_t) /* mtime */
	+ sizeof(uint64_t) /* ctime */
	+ sizeof(uint32_t) /* nlink */
	+ sizeof(uint32_t) /* blocks */
	+ user_len
	+ group_len;
}

off_t pack_stat(struct rfsd_instance *instance, char *buffer, struct stat *stbuf, int *ret)
{
	const char *user = get_uid_name(instance->id_lookup.uids, stbuf->st_uid);
	const char *group = get_gid_name(instance->id_lookup.gids, stbuf->st_gid);

#ifdef WITH_UGO
	if ((instance->server.mounted_export->options & OPT_UGO) != 0 
	&& (user == NULL
	|| group == NULL))
	{
		*ret = ECANCELED;
	}
#endif
	
	if (user == NULL)
	{
		user = "";
	}
	if (group == NULL)
	{
		group = "";
	}
	
	DEBUG("sending user: %s, group: %s\n", user, group);
	
	uint32_t mode = stbuf->st_mode;
	uint32_t user_len = strlen(user) + 1;
	uint32_t group_len = strlen(group) + 1;
	
	uint64_t size = stbuf->st_size;
	uint64_t atime = stbuf->st_atime;
	uint64_t mtime = stbuf->st_mtime;
	uint64_t ctime = stbuf->st_ctime;
	uint32_t nlink = stbuf->st_nlink;
	uint32_t blocks = stbuf->st_blocks;

	return 
	pack(group, group_len, buffer, 
	pack(user, user_len, buffer, 
	pack_32(&blocks, buffer, 
	pack_32(&nlink, buffer, 
	pack_64(&ctime, buffer, 
	pack_64(&mtime, buffer, 
	pack_64(&atime, buffer, 
	pack_64(&size, buffer, 
	pack_32(&group_len, buffer, 
	pack_32(&user_len, buffer, 
	pack_32(&mode, buffer, 0
	)))))))))));
}

int os_file_flags(uint16_t rfs_flags)
{
	int flags = 0;

	if (rfs_flags & RFS_APPEND)   { flags |= O_APPEND; }
	if (rfs_flags & RFS_ASYNC)    { flags |= O_ASYNC; }
	if (rfs_flags & RFS_CREAT)    { flags |= O_CREAT; }
	if (rfs_flags & RFS_EXCL)     { flags |= O_EXCL; }
	if (rfs_flags & RFS_NONBLOCK) { flags |= O_NONBLOCK; }
	if (rfs_flags & RFS_NDELAY)   { flags |= O_NDELAY; }
	if (rfs_flags & RFS_SYNC)     { flags |= O_SYNC; }
	if (rfs_flags & RFS_TRUNC)    { flags |= O_TRUNC; }
	if (rfs_flags & RFS_RDONLY)   { flags |= O_RDONLY; }
	if (rfs_flags & RFS_WRONLY)   { flags |= O_WRONLY; }
	if (rfs_flags & RFS_RDWR)     { flags |= O_RDWR; }

	return flags;
}

