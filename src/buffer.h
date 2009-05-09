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

/** allocate buffer of specified size */
static inline void* get_buffer(const size_t size)
{
	return malloc(size);
}

/** free buffer */
static inline void free_buffer(void *buffer)
{
	free(buffer);
}

/** copy data to buffer */
static inline off_t pack(const void *data, const size_t size, char *buffer, const off_t offset)
{
	memcpy(buffer + offset, data, size);
	return offset + size;
}

/** copy uint16_t to buffer 
@return offset after appended data
*/
static inline off_t pack_16(const uint16_t *data, char *buffer, const off_t offset)
{
	*((uint16_t *)(buffer + offset)) = htons(*data);
	return offset + sizeof(*data);
}

/** copy uint32_t to buffer 
@return offset after appended data
*/
static inline off_t pack_32(const uint32_t *data, char *buffer, const off_t offset)
{
	*((uint32_t *)(buffer + offset)) = htonl(*data);
	return offset + sizeof(*data);
}

/** same as pack_32, but for signed int */
static inline off_t pack_32_s(const int32_t *data, char *buffer, const off_t offset)
{
	*((int32_t *)(buffer + offset)) = htonl(*data);
	return offset + sizeof(*data);
}

/** copy uint64_t to buffer 
@return offset after appended data
*/
static inline off_t pack_64(const uint64_t *data, char *buffer, const off_t offset)
{
	*((uint64_t *)(buffer + offset)) = htonll(*data);
	return offset + sizeof(*data);
}

/** copy data from buffer 
@return offset after copied data
*/
static inline off_t unpack(void *data, const size_t size, const char *buffer, const off_t offset)
{
	memcpy(data, buffer + offset, size);
	return offset + size;
}

/** copy uint16_t from buffer 
@return offset after copied data
*/
static inline off_t unpack_16(uint16_t *data, const char *buffer, const off_t offset)
{
	*data = ntohs(*((uint16_t *)(buffer + offset)));
	return offset + sizeof(*data);
}

/** copy uint32_t from buffer 
@return offset after copied data
*/
static inline off_t unpack_32(uint32_t *data, const char *buffer, const off_t offset)
{
	*data = ntohl(*((uint32_t *)(buffer + offset)));
	return offset + sizeof(*data);
}

/** same as unpack_32, but for signed int */
static inline off_t unpack_32_s(int32_t *data, const char *buffer, const off_t offset)
{
	*data = ntohl(*((int32_t *)(buffer + offset)));
	return offset + sizeof(*data);
}

/** copy uint64_t from buffer 
@return offset after copied data
*/
static inline off_t unpack_64(uint64_t *data, const char *buffer, const off_t offset)
{
	*data = ntohll(*((uint64_t *)(buffer + offset)));
	return offset + sizeof(*data);
}

#ifdef RFS_DEBUG
/** print buffer to output. debug only */
void dump(const void *data, const size_t data_len);
#endif

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* BUFFER_H */

