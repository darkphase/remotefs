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

#include "../buffer.h"
#include "../config.h"
#include "../defines.h"
#include "../exports.h"
#include "../instance_server.h"
#include "utils.h"

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

char* pack_stat(struct stat *stbuf, char *buffer)
{
	uint32_t mode      = stbuf->st_mode;
	uint64_t size      = stbuf->st_size;
	uint64_t atime     = stbuf->st_atime;
	uint64_t mtime     = stbuf->st_mtime;
	uint64_t ctime     = stbuf->st_ctime;
	uint32_t nlink     = stbuf->st_nlink;
	uint32_t blocks    = stbuf->st_blocks;
	uint64_t ino       = stbuf->st_ino;

	pack_32(&blocks, 
	pack_32(&nlink, 
	pack_64(&ctime, 
	pack_64(&mtime, 
	pack_64(&atime, 
	pack_64(&size, 
	pack_32(&mode,
	pack_64(&ino, buffer
	))))))));

#ifdef RFS_DEBUG
	dump(buffer, STAT_BLOCK_SIZE);
#endif

	return buffer + STAT_BLOCK_SIZE;
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
