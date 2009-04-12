/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <string.h>

#include "buffer.h"
#include "config.h"
#include "names.h"

char* extract_server(const char *full_name)
{
	const char *delim = strchr(full_name, '@');
	if (delim == NULL)
	{
		return NULL;
	}

	size_t server_len = strlen(delim + 1) + 1;

	char *server = get_buffer(server_len);
	memcpy(server, delim + 1, server_len);

	return server;
}

char* extract_name(const char *full_name)
{
	const char *delim = strchr(full_name, '@');

	if (delim == NULL)
	{
		return NULL;
	}

	size_t name_len = (delim - full_name) + 1;

	char *name = get_buffer(name_len);
	memcpy(name, full_name, name_len - 1);
	name[name_len - 1] = 0;

	return name;
}

unsigned is_nss_name(const char *name)
{
	return (strchr(name, '@') == NULL ? 0 : 1);
}

