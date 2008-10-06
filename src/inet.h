/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef INET_H
#define INET_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/** htonll() support */

#include <netinet/in.h>

#if defined SOLARIS
#	include <sys/byteorder.h>
#	ifndef htonll
#		if defined(_BIG_ENDIAN)
#			define htonll(x) (x)
#		elif defined(_LITTLE_ENDIAN)
#			define htonll(x) BSWAP_64(x)
#		else
#			error "unsupported BYTE_ORDER"
#		endif /* __BYTE_ORDER */
#	endif /* htonll */

#	ifndef ntohll
#		if defined(_BIG_ENDIAN)
#			define ntohll(x) (x)
#		elif defined(_LITTLE_ENDIAN)
#			define ntohll(x) BSWAP_64(x)
#		else
#			error "unsupported BYTE_ORDER"
#		endif /* __BYTE_ORDER */
#	endif /* ntohll */

#elif defined FREEBSD
#	include <sys/endian.h>
#	ifndef htonll
#		if _BYTE_ORDER == _BIG_ENDIAN
#			define htonll(x) (x)
#		elif _BYTE_ORDER == _LITTLE_ENDIAN
#			define htonll(x) bswap64(x)
#		else
#			error "unsupported _BYTE_ORDER"
#		endif /* _BYTE_ORDER */
#	endif /* htonll */

#	ifndef ntohll
#		if _BYTE_ORDER == _BIG_ENDIAN
#			define ntohll(x) (x)
#		elif _BYTE_ORDER == _LITTLE_ENDIAN
#			define ntohll(x) bswap64(x)
#		else
#			error "unsupported _BYTE_ORDER"
#		endif /* _BYTE_ORDER */
#	endif /* ntohll */
#else /* Linux, ... */
#	include <endian.h>


#	ifndef htonll
#		if __BYTE_ORDER == __BIG_ENDIAN
#			define htonll(x) (x)
#		elif __BYTE_ORDER == __LITTLE_ENDIAN
#			include <byteswap.h>
#			define htonll(x) bswap_64(x)
#		else
#			error "unsupported __BYTE_ORDER"
#		endif /* __BYTE_ORDER */
#	endif /* htonll */

#	ifndef ntohll
#		if __BYTE_ORDER == __BIG_ENDIAN
#			define ntohll(x) (x)
#		elif __BYTE_ORDER == __LITTLE_ENDIAN
#			include <byteswap.h>
#			define ntohll(x) bswap_64(x)
#		else
#			error "unsupported __BYTE_ORDER"
#		endif /* __BYTE_ORDER */
#	endif /* ntohll */
#endif /* Linux, ... */

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* INET_H */
