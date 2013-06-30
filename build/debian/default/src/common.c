/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <string.h>
#include <sys/types.h>

#include "common.h"
#include "config.h"

char* socket_name(uid_t uid)
{
	char filename[FILENAME_MAX + 1] = { 0 };

	if (uid == (uid_t)-1)
	{
		return strdup(SHARED_SOCKET);
	}
	else
	{
		snprintf(filename, FILENAME_MAX + 1, SOCKETS_PATTERN, uid); /* TODO: check result */
		return strdup(filename);
	}
}

