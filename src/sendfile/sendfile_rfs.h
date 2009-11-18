/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include "../options.h"

#ifdef SENDFILE_AVAILABLE

#ifndef SENDFILE_H
#define SENDFILE_H

#include <sys/types.h>

/** generalized interface 
implementation should set errno to specific error (if any) 
generally this means that implementation just should manipulate errno 
after system's sendfile() is called 

\return size sent on success, -1 on error
*/
static inline ssize_t rfs_sendfile(int out_fd, int in_fd, off_t offset, size_t size);

#if (defined LINUX || defined SOLARIS)
#	include "sendfile_linux.h"
#elif (defined FREEBSD)
#	include "sendfile_freebsd.h"
#endif

#endif /* SENDFILE_H */
#endif /* SENDFILE_AVAILABLE */

