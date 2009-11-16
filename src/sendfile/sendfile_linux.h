/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#if ((defined LINUX || defined SOLARIS) && defined SENDFILE_AVAILABLE)

#ifndef SENDFILE_LINUX_H
#define SENDFILE_LINUX_H

#include <sys/sendfile.h>

static inline ssize_t rfs_sendfile(int out_fd, int in_fd, off_t offset, size_t size)
{
	return sendfile(out_fd, in_fd, &offset, size);
}

#endif /* SENDFILE_LINUX_H */
#endif /* LINUX && SENDFILE_AVAILABLE */

