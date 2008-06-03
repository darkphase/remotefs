#include "alloc.h"

#include <stdlib.h>

#include "config.h"

static char *last_allocated = NULL;
static unsigned last_allocated_size = 0;
static int freed = 0;

void *mp_alloc(unsigned size)
{
	if (freed != 0
	&& last_allocated != NULL)
	{
		if (size == last_allocated_size)
		{
			freed = 0;
			return last_allocated;
		}
		else
		{
			free(last_allocated);
		}
	}
	
	last_allocated = malloc(size);
	last_allocated_size = (last_allocated != NULL ? size : 0);
	freed = 0;
	
	return last_allocated;
}

void mp_free(void *buffer)
{
	if (buffer == last_allocated)
	{
		freed = 1;
	}
	else
	{
		free(buffer);
	}
}

void mp_force_free()
{
	if (last_allocated != NULL
	&& freed != 0)
	{
		free(last_allocated);
		last_allocated = NULL;
		last_allocated_size = 0;
		freed = 1;
	}
}
