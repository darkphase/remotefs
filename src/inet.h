/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef INET_H
#define INET_H

/** htonll() support */

#include <netinet/in.h>

#if defined SOLARIS
#	include <sys/byteorder.h>
#elif defined FREEBSD
#       include <sys/endian.h>
#else
#	include <endian.h>
#endif

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#ifndef htonll
#	if __BYTE_ORDER == __BIG_ENDIAN
#		define htonll(x) (x)
#	elif __BYTE_ORDER == __LITTLE_ENDIAN
#		include <byteswap.h>
#		define htonll(x) bswap_64(x)
#	else
#		error "unsupported __BYTE_ORDER"
#	endif /* __BYTE_ORDER */
#endif /* htonll */

#ifndef ntohll
#	if __BYTE_ORDER == __BIG_ENDIAN
#		define ntohll(x) (x)
#	elif __BYTE_ORDER == __LITTLE_ENDIAN
#		include <byteswap.h>
#		define ntohll(x) bswap_64(x)
#	else
#		error "unsupported __BYTE_ORDER"
#	endif /* __BYTE_ORDER */
#endif /* ntohll */

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* INET_H */
