/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef SERVER_HANDLERS_UTILS_H
#define SERVER_HANDLERS_UTILS_H

#include <sys/types.h>

struct rfsd_instance;
struct stat;

/** stat file */
int stat_file(struct rfsd_instance *instance, const char *path, struct stat *stbuf);

/** return stat block size as string length (with resolved user and group) */
size_t stat_size(struct rfsd_instance *instance, struct stat *stbuf, int *ret);

/** pack stat block to buffer */
off_t pack_stat(struct rfsd_instance *instance, char *buffer, struct stat *stbuf, int *ret);

/** convert rfs' file flags (RFS_RDWR, etc) to OS file flags (O_RDWR, etc) */
int os_file_flags(uint16_t rfs_flags);

#endif /* SERVER_HANDLERS_UTILS_H */

