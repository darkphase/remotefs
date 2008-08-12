#include "path.h"

#include <string.h>

int path_join(char full_path[NAME_MAX], const char *path, const char *filename)
{
	unsigned path_len = strlen(path);
	unsigned filename_len = strlen(filename);
	
	if (path_len + filename_len + 1 > NAME_MAX)
	{
		return -1;
	}

	memset(full_path, 0, NAME_MAX);
		
	const unsigned char copy_path = 
	(strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0 )
	? 0
	: 1;
	const unsigned char add_slash = (copy_path == 1 && path[path_len - 2] == '/' ? 0 : 1);
	
	if (copy_path == 1)
	{
		memcpy(full_path, path, path_len - 1);
	}
	
	if (copy_path == 1
	&& add_slash == 1)
	{
		memcpy(full_path + path_len - 1, "/", 1);
	}
	
	memcpy(full_path + (copy_path == 1 ? path_len - 1 + add_slash : 0), 
	filename, 
	strlen(filename));
		return 0;}
