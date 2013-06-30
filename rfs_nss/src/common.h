/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef COMMON_H
#define COMMON_H

/** routines common for client and server */

#include <sys/types.h>

char* socket_name(uid_t uid);

#endif /* COMMON_H */

