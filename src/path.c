/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <string.h>

#include "config.h"

int path_join(char *full_path, size_t max_len, const char *path, const char *filename)
{
	unsigned path_len = strlen(path);
	unsigned filename_len = strlen(filename);
	
	const unsigned add_slash = ((path[path_len - 1] == '/') ? 0 : 1);
	
	if (path_len + filename_len + add_slash + 1 > max_len)
	{
		return -E2BIG;
	}

	memcpy(full_path, path, path_len);
	
	if (add_slash != 0)
	{
		full_path[path_len] = '/';
	}
	
	memcpy(full_path + path_len + add_slash, 
		filename, 
		filename_len);

	full_path[path_len + add_slash + filename_len] = 0;

	return path_len + add_slash + filename_len;
}
