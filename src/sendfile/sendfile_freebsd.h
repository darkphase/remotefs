/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef SENDFILE_FREEBSD_H
#define SENDFILE_FREEBSD_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>

static inline ssize_t rfs_sendfile(int out_fd, int in_fd, off_t offset, size_t size)
{
	off_t done = 0;
	return (sendfile(in_fd, out_fd, offset, size, NULL, &done, 0) == 0 ? done : -1);
}

#endif /* SENDFILE_FREEBSD_H */
