/*
remotefs file system
See the file AUTHORS for copyright information.

This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include "../options.h"

#if defined RFSNSS_AVAILABLE

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "../buffer.h"
#include "../command.h"
#include "../config.h"
#include "../list.h"
#include "../names.h"

static char* find_socket(uid_t uid, const char *rfsd_host, int skip)
{
	char *ret = NULL;
	int skipped = 0;

	DEBUG("sockets dir: %s\n", NSS_SOCKETS_DIR);

	DIR *dir = opendir(NSS_SOCKETS_DIR);
	struct dirent *entry = NULL;

	char socket_pattern[FILENAME_MAX + 1] =  { 0 };
	snprintf(socket_pattern, sizeof(socket_pattern), "rfs_nss-%d-%s", uid, rfsd_host);

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

			size_t overall_len = strlen(NSS_SOCKETS_DIR) + strlen(entry->d_name) + 1;

			ret = malloc(overall_len);
			snprintf(ret, overall_len, "%s%s", NSS_SOCKETS_DIR, entry->d_name);
			ret[overall_len - 1] = 0;

			break;
		}
	}

	closedir(dir);

	return ret;
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

		int sock = socket(AF_UNIX, SOCK_STREAM, 0);

		if (sock == -1)
		{
			last_errno = errno;
			free(socket_name);
			break;
		}

		struct sockaddr_un address = { 0 };
		strcpy(address.sun_path, socket_name);
		address.sun_family = AF_UNIX;

		free(socket_name);

		DEBUG("%s\n", "connecting");

		if (connect(sock, (struct sockaddr *)&address, sizeof(address)) != 0)
		{
			close(sock);
			continue;
		}

		return sock;
	}

	return -last_errno;
}

static int check_name(const char *full_name, enum rfs_commands cmd_id)
{
	char *server = extract_server(full_name);
	if (server == NULL)
	{
		return -EINVAL;
	}

	int sock = nss_connect(server);
	if (sock < 0)
	{
		free(server);
		return sock;
	}

	free(server);

	int saved_errno = 0;

	char *name = extract_name(full_name);

	size_t overall_size = 0;
	struct rfs_command cmd = { 0 };
	struct rfs_answer ans = { 0 };

	if (name == NULL)
	{
		saved_errno = -EINVAL;
		goto error;
	}

	overall_size = strlen(name) + 1;
	cmd.command = cmd_id;
	cmd.data_len = overall_size;

#ifdef RFS_DEBUG
	dump_command(&cmd);
#endif

	if (send(sock, &cmd, sizeof(cmd), 0) != sizeof(cmd))
	{
		saved_errno = errno;
		free(name);
		goto error;
	}

	DEBUG("sending name: %s\n", name);

	if (send(sock, name, overall_size, 0) != overall_size)
	{
		saved_errno = errno;
		free(name);
		goto error;
	}

	free(name);

	if (recv(sock, &ans, sizeof(ans), 0) != sizeof(ans))
	{
		saved_errno = errno;
		goto error;
	}

#ifdef RFS_DEBUG
	dump_answer(&ans);
#endif

	if (ans.command != cmd_id)
	{
		saved_errno = EINVAL;
		goto error;
	}

	saved_errno = ans.ret_errno;

error:
	shutdown(sock, SHUT_RDWR);
	close(sock);

	return -saved_errno;

}

int nss_check_user(const char *full_name)
{
	return check_name(full_name, cmd_checkuser);
}

int nss_check_group(const char *full_name)
{
	return check_name(full_name, cmd_checkgroup);
}

static int get_names(const char *server, struct rfs_list **names, enum rfs_commands cmd_id)
{
	if (*names != NULL)
	{
		destroy_list(names);
	}

	int sock = nss_connect(server);
	if (sock < 0)
	{
		return sock;
	}

	int saved_errno = 0;

	struct rfs_command cmd = { cmd_id, 0 };
	struct rfs_answer ans = { 0 };

#ifdef RFS_DEBUG
	dump_command(&cmd);
#endif

	if (send(sock, &cmd, sizeof(cmd), 0) != sizeof(cmd))
	{
		saved_errno = errno;
		goto error;
	}

	do
	{
		if (recv(sock, &ans, sizeof(ans), 0) != sizeof(ans))
		{
			saved_errno = errno;
			goto error;
		}

#ifdef RFS_DEBUG
		dump_answer(&ans);
#endif

		if (ans.command != cmd_id)
		{
			saved_errno = EINVAL;
			goto error;
		}

		if (ans.data_len > 0)
		{
			char *name = malloc(ans.data_len);

			if (recv(sock, name, ans.data_len, 0) != ans.data_len)
			{
				saved_errno = errno;
				goto error;
			}

			if (add_to_list(names, name) == NULL)
			{
				saved_errno = EIO;
				goto error;
			}
		}
	}
	while (ans.data_len != 0
	&& ans.ret_errno == 0);

	goto success;

error:
	destroy_list(names);

success:
	shutdown(sock, SHUT_RDWR);
	close(sock);

	return saved_errno;
}

int nss_get_users(const char *server, struct rfs_list **users)
{
	return get_names(server, users, cmd_getusers);
}

int nss_get_groups(const char *server, struct rfs_list **groups)
{
	return get_names(server, groups, cmd_getgroups);
}

#else
int nss_client_c_empty_module_makes_suncc_feel_bad = 0;
#endif /* RFSNSS_AVAILABLE */

