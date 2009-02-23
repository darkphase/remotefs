/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>

#include "config.h"
#include "command.h"
#include "buffer.h"

static char* find_socket(uid_t uid, const char *rfsd_host, int skip)
{
	char *ret = NULL;
	int skipped = 0;

	DEBUG("sockets dir: %s\n", NSS_SOCKETS_DIR);

	DIR *dir = opendir(NSS_SOCKETS_DIR);
	struct dirent *entry = NULL;

	char socket_pattern[FILENAME_MAX + 1] =  { 0 };
	snprintf(socket_pattern, sizeof(socket_pattern), "%d-%s", uid, rfsd_host);

	DEBUG("socket pattern: %s\n", socket_pattern);

	while (1)
	{
		entry = readdir(dir);
		if (entry == NULL)
		{
			break;
		}

		if (strstr(entry->d_name, socket_pattern) == entry->d_name)
		{
			DEBUG("file is matching pattern: %s\n", entry->d_name);
			if (skipped < skip)
			{
				DEBUG("%s\n", "skipping");
				++skipped;
				continue;
			}

			ret = get_buffer(strlen((const char *)entry->d_name) + 1);
			memcpy(ret, (const char *)entry->d_name, strlen((const char *)entry->d_name) + 1);
			break;
		}
	}
	
	closedir(dir);

	return ret;
}

static char* extract_server(const char *full_name)
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

static char* extract_name(const char *full_name)
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

static int nss_connect(const char *server)
{
	int last_errno = 0;
	int skip = -1;

	while (1)
	{
		++skip;

		char *socket_name = find_socket(getuid(), server, skip);
		if (socket_name == NULL)
		{
			return -EAGAIN;
		}

		DEBUG("socket name: %s\n", socket_name);

		int sock = socket(PF_UNIX, SOCK_STREAM, 0);

		if (sock == -1)
		{
			last_errno = errno;
			free_buffer(socket_name);
			continue;
		}
		
		struct sockaddr_un address = { 0 };
		strcpy(address.sun_path, socket_name);
		address.sun_family = AF_UNIX;
	
		free_buffer(socket_name);
	
		DEBUG("%s\n", "connecting");

		if (connect(sock, (struct sockaddr *)&address, sizeof(address)) != 0)
		{
			last_errno = errno;
			close(sock);
			continue;
		}

		return sock;
	}

	return -last_errno;
}

static int check_name(const char *full_name, enum server_commands cmd_id)
{	
	char *server = extract_server(full_name);
	if (server == NULL)
	{
		return -EINVAL;
	}

	int sock = nss_connect(server);
	if (sock < 0)
	{
		free_buffer(server);
		return sock;
	}
			
	free_buffer(server);
		
	int saved_errno = 0;
	
	char *name = extract_name(full_name);
	if (name == NULL)
	{
		saved_errno = -EINVAL;
		goto error;
	}

	size_t overall_size = strlen(name) + 1;
	struct command cmd = { cmd_id, overall_size };

	dump_command(&cmd);

	if (send(sock, &cmd, sizeof(cmd), 0) != sizeof(cmd))
	{
		saved_errno = errno;
		free_buffer(name);
		goto error;
	}

	DEBUG("sending data: %s\n", name);

	if (send(sock, name, overall_size, 0) != overall_size)
	{
		saved_errno = errno;
		free_buffer(name);
		goto error;
	}
		
	free_buffer(name);
		
	DEBUG("%s\n", "getting result");

	uint32_t ret_errno = 0;

	if (recv(sock, &ret_errno, sizeof(ret_errno), 0) != sizeof(ret_errno))
	{
		saved_errno = errno;
		goto error;
	}
	
	saved_errno = ret_errno;

error:
	shutdown(sock, SHUT_RDWR);
	close(sock);

	return -saved_errno;

}

int nss_check_user(const char *full_name)
{
	int ret = check_name(full_name, cmd_checkuser);
	
	return ret;
}

int nss_check_group(const char *full_name)
{
	int ret = check_name(full_name, cmd_checkgroup);
	
	return ret;
}

