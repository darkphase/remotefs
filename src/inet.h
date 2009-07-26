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
#        include <inttypes.h>

#elif defined FREEBSD
#        include <sys/endian.h>
#        define RFS_BSWAP_FUNC bswap64
#
#        if _BYTE_ORDER == _BIG_ENDIAN
#                define RFS_BIG_ENDIAN
#        elif _BYTE_ORDER == _LITTLE_ENDIAN
#                define RFS_LITTLE_ENDIAN
#        endif

#elif defined DARWIN
#        include <machine/endian.h>
#        define RFS_BSWAP_FUNC OSSwapConstInt64
#
#        if __DARWIN_BYTE_ORDER == __DARWIN_BIG_ENDIAN
#                define RFS_BIG_ENDIAN
#        elif __DARWIN_BYTE_ORDER == __DARWIN_LITTLE_ENDIAN
#                define RFS_LITTLE_ENDIAN
#        endif

#elif defined QNX
#       include <gulliver.h>
#       include <arpa/inet.h>
#       if defined __LITTLEENDIAN__
#               define RFS_LITTLE_ENDIAN
#       else
#               define RFS_BIG_ENDIAN
#       endif
#       define RFS_BSWAP_FUNC ENDIAN_RET64

#else /* Linux */
#        include <endian.h>
#        include <byteswap.h>
#        define RFS_BSWAP_FUNC bswap_64
#
#        if __BYTE_ORDER == __BIG_ENDIAN
#                define RFS_BIG_ENDIAN
#        elif __BYTE_ORDER == __LITTLE_ENDIAN
#                define RFS_LITTLE_ENDIAN
#        endif
#endif

/* actual htonll support */

#if ! defined SOLARIS

#if defined RFS_BIG_ENDIAN
#	define htonll(x) (x)
#elif defined RFS_LITTLE_ENDIAN
#	define htonll(x) RFS_BSWAP_FUNC(x)
#else
#	error "unsupported BYTE_ORDER"
#endif

#if defined RFS_BIG_ENDIAN
#	define ntohll(x) (x)
#elif defined RFS_LITTLE_ENDIAN
#	define ntohll(x) RFS_BSWAP_FUNC(x)
#else
#	error "unsupported BYTE_ORDER"
#endif

#endif /* if ! defined SOLARIS */

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* INET_H */

