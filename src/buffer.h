/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef BUFFER_H
#define BUFFER_H

/** memory allocation routines */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "inet.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/** copy data to buffer */
static inline char* pack(const void *data, const size_t size, char *buffer)
{
	memcpy(buffer, data, size);
	return buffer + size;
}

/** copy uint16_t to buffer 
@return offset after appended data
*/
static inline char* pack_16(const uint16_t *data, char *buffer)
{
	*((uint16_t *)buffer) = htons(*data);
	return buffer + sizeof(*data);
}

/** same as pack_16, but for signed int */
static inline char* pack_16_s(const int16_t *data, char *buffer)
{
	*((int16_t *)buffer) = htons(*data);
	return buffer + sizeof(*data);
}

/** copy uint32_t to buffer 
@return offset after appended data
*/
static inline char* pack_32(const uint32_t *data, char *buffer)
{
	*((uint32_t *)buffer) = htonl(*data);
	return buffer + sizeof(*data);
}

/** same as pack_32, but for signed int */
static inline char* pack_32_s(const int32_t *data, char *buffer)
{
	*((int32_t *)buffer) = htonl(*data);
	return buffer + sizeof(*data);
}

/** copy uint64_t to buffer 
@return offset after appended data
*/
static inline char* pack_64(const uint64_t *data, char *buffer)
{
	*((uint64_t *)buffer) = htonll(*data);
	return buffer + sizeof(*data);
}

/** same as pack_64, but for signed int */
static inline char* pack_64_s(const int64_t *data, char *buffer)
{
	*((int64_t *)buffer) = htonll(*data);
	return buffer + sizeof(*data);
}

/** copy data from buffer 
@return offset after copied data
*/
static inline const char* unpack(void *data, const size_t size, const char *buffer)
{
	memcpy(data, buffer, size);
	return buffer + size;
}

/** copy uint16_t from buffer 
@return offset after copied data
*/
static inline const char* unpack_16(uint16_t *data, const char *buffer)
{
	*data = ntohs(*(uint16_t *)buffer);
	return buffer + sizeof(*data);
}

/** same as unpack_16, but for signed int */
static inline const char* unpack_16_s(int16_t *data, const char *buffer)
{
	*data = ntohs(*(int16_t *)buffer);
	return buffer + sizeof(*data);
}

/** copy uint32_t from buffer 
@return offset after copied data
*/
static inline const char* unpack_32(uint32_t *data, const char *buffer)
{
	*data = ntohl(*((uint32_t *)buffer));
	return buffer + sizeof(*data);
}

/** same as unpack_32, but for signed int */
static inline const char* unpack_32_s(int32_t *data, const char *buffer)
{
	*data = ntohl(*((int32_t *)buffer));
	return buffer + sizeof(*data);
}

/** copy uint64_t from buffer 
@return offset after copied data
*/
static inline const char* unpack_64(uint64_t *data, const char *buffer)
{
	*data = ntohll(*((uint64_t *)buffer));
	return buffer + sizeof(*data);
}

/** same as unpack_64, but for signed int */
static inline const char* unpack_64_s(int64_t *data, const char *buffer)
{
	*data = ntohll(*((int64_t *)buffer));
	return buffer + sizeof(*data);
}

/** duplicate buffer using get_buffer() */
char* buffer_dup(const char *buffer, size_t buffer_len);

/** duplicate buffer using get_buffer(strlen + 1) and set trailing 0 to the str_len + 1 */
char* buffer_dup_str(const char *buffer, size_t str_len);

#ifdef RFS_DEBUG
/** print buffer to output. debug only */
void dump(const void *data, const size_t data_len);
#endif

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* BUFFER_H */
