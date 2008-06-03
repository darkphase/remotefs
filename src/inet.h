#ifndef INET_H
#define INET_H

#include <arpa/inet.h>
#include <endian.h>

#ifndef htonll
	#if __BYTE_ORDER == __BIG_ENDIAN
		#define htonll(x) (x)
	#elif __BYTE_ORDER == __LITTLE_ENDIAN
		#include <byteswap.h>
		#define htonll(x) bswap_64(x)
	#else
		#error "unsupported __BYTE_ORDER"
	#endif // __BYTE_ORDER
#endif // htonll

#ifndef ntohll
	#if __BYTE_ORDER == __BIG_ENDIAN
		#define ntohll(x) (x)
	#elif __BYTE_ORDER == __LITTLE_ENDIAN
		#include <byteswap.h>
		#define ntohll(x) bswap_64(x)
	#else
		#error "unsupported __BYTE_ORDER"
	#endif // __BYTE_ORDER
#endif // ntohll

#endif // INET_H
