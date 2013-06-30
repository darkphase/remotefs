/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"
#include "global_lock.h"

int get_global_lock(struct config *config)
{
	DEBUG("%s\n", "aquiring global lock");

	int fd = open(GLOBAL_LOCK, 
		O_CREAT | O_RDWR, 
		S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

	if (fd == -1)
	{
		return -errno;
	}

	if (lockf(fd, F_LOCK, 0) == 0)
	{
		config->lock = fd;
	}
	
	DEBUG("%s\n", "locked");

	/* this is just nice stuff, lock is aquired and will work without it
	so ignore errors */
	if (ftruncate(fd, 0) == 0)
	{
		char str_pid[64] = { 0 };
		if (snprintf(str_pid, sizeof(str_pid), "%u", getpid()) != sizeof(str_pid))
		{
			if (write(fd, str_pid, strlen(str_pid)) == strlen(str_pid))
			{
				fsync(fd);
			}
		}
	}

	return 0;
}

int release_global_lock(struct config *config)
{
	DEBUG("%s\n", "releasing global lock");

	if (config->lock <= 0)
	{
		return -EINVAL;
	}

	if (unlink(GLOBAL_LOCK) != 0)
	{
		return -errno;
	}

	if (lockf(config->lock, F_ULOCK, 0) != 0)
	{
		return -errno;
	}
	
	DEBUG("%s\n", "released");

	close(config->lock); /* ignore error - we're already released actual lock */
	config->lock = -1;

	return 0;
}

int try_global_lock(struct config *config)
{
	if (config->lock <= 0)
	{
		return -EINVAL;
	}

	if (lockf(config->lock, F_TLOCK, 0) != 0)
	{
		return -errno;
	}

	return 0;
}

unsigned got_global_lock(const struct config *config)
{
	return (config->lock != -1 ? 1 : 0);
}

