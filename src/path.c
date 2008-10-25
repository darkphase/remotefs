/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <string.h>

#include "config.h"
#include "path.h"

int path_join(char *full_path, size_t max_len, const char *path, const char *filename)
{
	unsigned path_len = strlen(path);
	unsigned filename_len = strlen(filename);
	
	const unsigned char copy_path = 
	(strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0 )
	? 0
	: 1;
	const unsigned char add_slash = ((copy_path == 1 && path[path_len - 1] == '/') ? 0 : 1);
	
	if (path_len + filename_len + add_slash + 1 > max_len)
	{
		return -1;
	}

	memset(full_path, 0, max_len);
	
	if (copy_path == 1)
	{
		memcpy(full_path, path, path_len);
	}
	
	if (copy_path == 1
	&& add_slash == 1)
	{
		memcpy(full_path + path_len, "/", 1);
	}
	
	memcpy(full_path + (copy_path == 1 ? path_len + add_slash : 0), 
	filename, 
	strlen(filename));
	
	return 0;
}
