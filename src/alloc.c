#include "alloc.h"

#include <stdlib.h>

void *mp_alloc(size_t size)
{
	return malloc(size);
}

void mp_free(void *buffer)
{
	free(buffer);
}

void mp_force_free()
{
}
