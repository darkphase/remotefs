#ifndef BUFFER_H
#define BUFFER_H

#include <stdint.h>

void* get_buffer(const unsigned size);
void free_buffer(void *buffer);

unsigned pack(const void *data, const unsigned size, void *buffer, const unsigned offset);
unsigned pack_16(const uint16_t *data, void *buffer, const unsigned offset);
unsigned pack_32(const uint32_t *data, void *buffer, const unsigned offset);
unsigned pack_64(const uint64_t *data, void *buffer, const unsigned offset);

unsigned unpack(void *data, const unsigned size, const void *buffer, const unsigned offset);
unsigned unpack_16(uint16_t *data, const void *buffer, const unsigned offset);
unsigned unpack_32(uint32_t *data, const void *buffer, const unsigned offset);
unsigned unpack_64(uint64_t *data, const void *buffer, const unsigned offset);

void dump(const void *data, const unsigned data_len);

#endif // BUFFER_H
