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

static char* find_socket(uid_t uid, const char *rfsd_host)
{
	char *ret = NULL;

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

static int check_name(const char *name, const char *server, enum server_commands cmd_id)
{
	char *socket_name = find_socket(getuid(), server);
	if (socket_name == NULL)
	{
		return -EAGAIN;
	}

	DEBUG("socket name: %s\n", socket_name);

	int sock = socket(PF_UNIX, SOCK_STREAM, 0);
	if (sock == -1)
	{
		free_buffer(socket_name);
		return -errno;
	}

	struct sockaddr_un address = { 0 };
	strcpy(address.sun_path, socket_name);
	address.sun_family = AF_UNIX;
	
	free_buffer(socket_name);
	
	DEBUG("%s\n", "connecting");

	if (connect(sock, (struct sockaddr *)&address, sizeof(address)) != 0)
	{
		return -errno;
	}

	int saved_errno = 0;

	size_t overall_size = strlen(name) + 1;
	struct command cmd = { cmd_id, overall_size };

	DEBUG("%s\n", "sending command");

	if (send(sock, &cmd, sizeof(cmd), 0) != sizeof(cmd))
	{
		saved_errno = errno;
		goto error;
	}

	DEBUG("sending data: %s\n", name);

	if (send(sock, name, overall_size, 0) != overall_size)
	{
		saved_errno = errno;
		goto error;
	}
		
	DEBUG("%s\n", "getting result");

	uint32_t ret_errno = 0;

	if (recv(sock, &ret_errno, sizeof(ret_errno), 0) != sizeof(ret_errno))
	{
		saved_errno = errno;
		goto error;
	}
	
	saved_errno = ret_errno;

error:
	close(sock);
	shutdown(sock, SHUT_RDWR);

	return -saved_errno;

}

int nss_check_user(const char *full_name)
{
	char *name = extract_name(full_name);
	if (name == NULL)
	{
		return -EINVAL;
	}

	char *server = extract_server(full_name);
	if (server == NULL)
	{
		free_buffer(name);
		return -EINVAL;
	}

	int ret = check_name(name, server, cmd_checkuser);
	
	free_buffer(name);
	free_buffer(server);

	return ret;
}

int nss_check_group(const char *full_name)
{
	char *name = extract_name(full_name);
	if (name == NULL)
	{
		return -EINVAL;
	}

	char *server = extract_server(full_name);
	if (server == NULL)
	{
		free_buffer(name);
		return -EINVAL;
	}

	int ret = check_name(name, server, cmd_checkgroup);
	
	free_buffer(name);
	free_buffer(server);

	return ret;
}

