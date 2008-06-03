#include "keep_alive.h"

static unsigned char lock = 0;

int get_keep_alive_lock()
{
	if (lock == 0)
	{
		lock = 1;
		return 0;
	}
	
	return -1;
}

void release_keep_alive_lock()
{
	lock = 0;
}
