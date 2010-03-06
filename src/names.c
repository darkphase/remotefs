/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "config.h"
#include "instance_client.h"
#include "names.h"

char* extract_server(const char *full_name)
{
	const char *delim = strchr(full_name, '@');
	if (delim == NULL)
	{
		return NULL;
	}

	size_t server_len = strlen(delim + 1) + 1;

	char *server = malloc(server_len);
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

	char *name = malloc(name_len);
	memcpy(name, full_name, name_len - 1);
	name[name_len - 1] = 0;

	return name;
}

unsigned is_nss_name(const char *name)
{
	return (strchr(name, '@') == NULL ? 0 : 1);
}

char* local_nss_name(const char *full_name, const struct rfs_instance *instance)
{
	char *local_name = extract_name(full_name);
	if (local_name == NULL)
	{
		return NULL;
	}

	char *name_server = extract_server(full_name);
	if (name_server == NULL)
	{
		free(local_name);
		return NULL;
	}

	if (strcmp(name_server, instance->config.host) != 0)
	{
		free(local_name);
		free(name_server);
		return NULL;
	}

	free(name_server);

	return local_name;
}

char* remote_nss_name(const char *short_name, const struct rfs_instance *instance)
{
	size_t name_len = strlen(short_name);
	size_t hostname_len = strlen(instance->config.host);
	size_t overall_len = name_len + 1 + hostname_len;

	char *long_name = malloc(overall_len + 1);

	memcpy(long_name, short_name, name_len);
	memcpy(long_name + name_len + 1, instance->config.host, hostname_len);

	long_name[name_len] = '@';
	long_name[overall_len] = 0;

	return long_name;
}
