/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef DEFINES_H
#define DEFINES_H

/** remotefs defines for operations and handlers */

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/** flags for rfs_open() */
enum rfs_open_flags
{
	RFS_APPEND              = 1, 
	RFS_ASYNC               = 2,
	RFS_CREAT               = 4,
	RFS_EXCL                = 8,
	RFS_NONBLOCK            = 16,
	RFS_NDELAY              = 32,
	RFS_SYNC                = 64,
	RFS_TRUNC               = 128,
	RFS_RDONLY              = 512,
	RFS_WRONLY              = 1024,
	RFS_RDWR                = 2048
};

/** flags for rfs_lock() */
enum rfs_lock_flags
{
	RFS_GETLK               = 1,
	RFS_SETLK               = 2,
	RFS_SETLKW              = 4
};

/** types for rfs_lock() */
enum rfs_lock_type
{
	RFS_RDLCK               = 1,
	RFS_WRLCK               = 2,
	RFS_UNLCK               = 4
};

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* DEFINES_H */

