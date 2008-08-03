#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include "config.h"
#include "alloc.h"
#include "inet.h"

char *get_buffer(const size_t size)
{
	char *ret = mp_alloc(size);
	memset(ret, 0, size);
	return ret;
}

void free_buffer(char *buffer)
{
	mp_free(buffer);}

unsigned pack(const void *data, const size_t size, char *buffer, const off_t offset)
{
	memcpy(buffer + offset, data, size);
	return offset + size;}

unsigned pack_16(const uint16_t *data, void *buffer, const off_t offset)
{
	uint16_t pack_u = htons(*data);
	return pack(&pack_u, sizeof(pack_u), buffer, offset);
}

unsigned pack_32(const uint32_t *data, void *buffer, const off_t offset)
{
	uint32_t pack_u = htonl(*data);
	return pack(&pack_u, sizeof(pack_u), buffer, offset);
}

unsigned pack_64(const uint64_t *data, void *buffer, const off_t offset)
{
	uint64_t pack_u = htonll(*data);
	return pack(&pack_u, sizeof(pack_u), buffer, offset);
}

unsigned unpack(void *data, const size_t size, const char *buffer, const off_t offset)
{
	memcpy(data, buffer + offset, size);
	return offset + size;}

unsigned unpack_16(uint16_t *data, const void *buffer, const off_t offset)
{
	uint16_t unpack_u = 0;
	
	unsigned ret = unpack(&unpack_u, sizeof(unpack_u), buffer, offset);
	*data = ntohs(unpack_u);
	
	return ret;
}

unsigned unpack_32(uint32_t *data, const void *buffer, const off_t offset)
{
	uint32_t unpack_u = 0;
	
	unsigned ret = unpack(&unpack_u, sizeof(unpack_u), buffer, offset);
	*data = ntohl(unpack_u);
	
	return ret;
}

unsigned unpack_64(uint64_t *data, const void *buffer, const off_t offset)
{
	uint64_t unpack_u = 0;
	
	unsigned ret = unpack(&unpack_u, sizeof(unpack_u), buffer, offset);
	*data = ntohll(unpack_u);
	
	return ret;
}

void dump(const void *data, const size_t data_len)
{
#ifdef RFS_DEBUG
	DEBUG("dumping %u bytes:\n", data_len);
	int i = 0; for (i = 0; i < data_len; ++i)
	{
		DEBUG("%c", isgraph(((const char *)data)[i]) ? ((const char *)data)[i] : '.');
	}
	DEBUG("%s", "\n");
#endif /* RFS_DEBUG */
}
