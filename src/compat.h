/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef COMPAT_H
#define COMPAT_H

#if defined FREEBSD
#endif /* FREEBSD */

#if defined DARWIN
#endif /* DARWIN */

#if defined SOLARIS
#endif /* SOLARIS */

#if defined QNX
#endif /* QNX */

#if O_ASYNC == 0
#else 
#	if ! defined O_ASYNC
#		define O_ASYNC 0
#	endif
#endif

#endif /* COMPAT_H */

