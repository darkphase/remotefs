/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <ctype.h>

#include "buffer.h"
#include "config.h"

#ifdef RFS_DEBUG
void dump(const void *data, const size_t data_len)
{
	DEBUG("dumping %u bytes:\n", (unsigned int)data_len);
	int i = 0; for (i = 0; i < data_len; ++i)
	{
		DEBUG("%c", isgraph(((const char *)data)[i]) ? ((const char *)data)[i] : '.');
	}
	DEBUG("%s", "\n");
}
#endif /* RFS_DEBUG */

