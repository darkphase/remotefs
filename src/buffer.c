/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "config.h"
#include "inet.h"

void *get_buffer(const size_t size)
{
	return malloc(size);
}

void free_buffer(void *buffer)
{
	free(buffer);
}

off_t pack(const void *data, const size_t size, char *buffer, const off_t offset)
{
	memcpy(buffer + offset, data, size);
	return offset + size;
}

off_t pack_16(const uint16_t *data, char *buffer, const off_t offset)
{
	*((uint16_t *)(buffer + offset)) = htons(*data);
	return offset + sizeof(*data);
}

off_t pack_32(const uint32_t *data, char *buffer, const off_t offset)
{
	*((uint32_t *)(buffer + offset)) = htonl(*data);
	return offset + sizeof(*data);
}

off_t pack_32_s(const int32_t *data, char *buffer, const off_t offset)
{
	*((int32_t *)(buffer + offset)) = htonl(*data);
	return offset + sizeof(*data);
}

off_t pack_64(const uint64_t *data, char *buffer, const off_t offset)
{
	*((uint64_t *)(buffer + offset)) = htonll(*data);
	return offset + sizeof(*data);
}

off_t unpack(void *data, const size_t size, const char *buffer, const off_t offset)
{
	memcpy(data, buffer + offset, size);
	return offset + size;
}

off_t unpack_16(uint16_t *data, const char *buffer, const off_t offset)
{
	*data = ntohs(*((uint16_t *)(buffer + offset)));
	return offset + sizeof(*data);
}

off_t unpack_32(uint32_t *data, const char *buffer, const off_t offset)
{
	*data = ntohl(*((uint32_t *)(buffer + offset)));
	return offset + sizeof(*data);
}

off_t unpack_32_s(int32_t *data, const char *buffer, const off_t offset)
{
	*data = ntohl(*((int32_t *)(buffer + offset)));
	return offset + sizeof(*data);
}

off_t unpack_64(uint64_t *data, const char *buffer, const off_t offset)
{
	*data = ntohll(*((uint64_t *)(buffer + offset)));
	return offset + sizeof(*data);
}

void dump(const void *data, const size_t data_len)
{
#ifdef RFS_DEBUG
	DEBUG("dumping %u bytes:\n", (unsigned int)data_len);
	int i = 0; for (i = 0; i < data_len; ++i)
	{
		DEBUG("%c", isgraph(((const char *)data)[i]) ? ((const char *)data)[i] : '.');
	}
	DEBUG("%s", "\n");
#endif /* RFS_DEBUG */
}

