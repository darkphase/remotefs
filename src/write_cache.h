#ifndef WRITE_CACHE_H
#define WRITE_CACHE_H

#include <stdint.h>

struct list;

struct write_cache_entry
{
	uint64_t descriptor;
	char *buffer;
	unsigned size;
};

unsigned char is_fit_to_write_cache(unsigned size);

// will copy buffer
int add_to_write_cache(uint64_t descriptor, const char *buffer, unsigned size);
unsigned char is_file_in_write_cache(uint64_t descriptor);
const struct list* get_write_cache();
unsigned get_write_cache_size();
void destroy_write_cache();

#endif // WRITE_CACHE_H

