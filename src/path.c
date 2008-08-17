#include "path.h"

#include <string.h>
#include "config.h"

int path_join(char full_path[NAME_MAX], const char *path, const char *filename)
{
	unsigned path_len = strlen(path);
	unsigned filename_len = strlen(filename);
	
	DEBUG("%s %d\n", path, path_len);
	DEBUG("%s %d\n", filename, filename_len);
	
	if (path_len + filename_len + 1 > NAME_MAX)
	{
		return -1;
	}

	memset(full_path, 0, NAME_MAX);
	
	const unsigned char copy_path = 
	(strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0 )
	? 0
	: 1;
	const unsigned char add_slash = ((copy_path == 1 && path[path_len - 1] == '/') ? 0 : 1);
	
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
	
	return 0;}
