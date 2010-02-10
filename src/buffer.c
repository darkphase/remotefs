/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <ctype.h>
#include <stdlib.h>

#include "buffer.h"
#include "config.h"

char* buffer_dup(const char *buffer, size_t buffer_len)
{
	char *ret = malloc(buffer_len);
	if (ret != NULL)
	{
		memcpy(ret, buffer, buffer_len);
	}
	return ret;
}

char* buffer_dup_str(const char *buffer, size_t str_len)
{
	char *ret = malloc(str_len + 1);
	if (ret != NULL)
	{
		memcpy(ret, buffer, str_len);
		ret[str_len] = 0;
	}
	return ret;
}

#ifdef RFS_DEBUG
void dump(const void *data, const size_t data_len)
{
	DEBUG("dumping %u bytes:\n", (unsigned int)data_len);
	int i = 0; for (i = 0; i < data_len; ++i)
	{
		fprintf(stderr, "%c%s", isgraph(((const char *)data)[i]) ? ((const char *)data)[i] : '.', (i == data_len - 1 ? "\n" : ""));
	}
}
#endif /* RFS_DEBUG */

