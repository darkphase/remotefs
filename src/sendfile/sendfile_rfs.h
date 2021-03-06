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

#include <stdint.h>
#include <sys/types.h>

struct rfs_command;
struct rfsd_instance;

/** generalized interface 
implementation shouldn't set errno to specific error (if any) 
generally this means that implementation just shouldn't manipulate errno 
after system's sendfile() is called 

\return size sent on success, -1 on error
*/
static inline ssize_t rfs_sendfile(int out_fd, int in_fd, off_t offset, size_t size);

#if (defined LINUX || defined SOLARIS)
#	include "sendfile_linux.h"
#elif (defined FREEBSD)
#	include "sendfile_freebsd.h"
#endif

int read_with_sendfile(struct rfsd_instance *instance, const struct rfs_command *cmd, uint64_t handle, off_t offset, size_t size);

#endif /* SENDFILE_H */
#endif /* SENDFILE_AVAILABLE */

