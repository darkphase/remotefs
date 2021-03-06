/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef COMPAT_H
#define COMPAT_H

/** OS-dependent defines */

#if defined FREEBSD
#endif /* FREEBSD */

#if defined DARWIN
#endif /* DARWIN */

#if defined SOLARIS
#endif /* SOLARIS */

#if defined QNX
#include <sys/time.h>
#	if ! defined AI_ADDRCONFIG
#		define AI_ADDRCONFIG 0
#	endif
#	if ! defined IOV_MAX
#		define IOV_MAX _XOPEN_IOV_MAX
#	endif 
#endif /* QNX */

#if ! defined O_ASYNC
#	define O_ASYNC 0
#endif

#endif /* COMPAT_H */
