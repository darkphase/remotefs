/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include <buffer.h>
#include "client_common.h"
#include "config.h"
#include "common.h"
#include "nss_cmd.h"

static char* find_socket(uid_t uid)
{
	char *socketname = socket_name(uid);
	
	DEBUG("socket name should be %s\n", socketname);

	struct stat stbuf = { 0 };
	stat(socketname, &stbuf);

	if ((stbuf.st_mode & S_IFSOCK) != 0)
	{
		return socketname;
	}

	free(socketname);
	return NULL;
}

int make_connection(uid_t uid)
{
	char *socketname = find_socket(uid);

	DEBUG("socket found: %s\n", socketname == NULL ? "NULL" : socketname);

	if (socketname == NULL)
	{
		return -EAGAIN;
	}

	int sock = socket(PF_UNIX, SOCK_STREAM, 0);

	if (sock == -1)
	{
		free(socketname);
		return -errno;
	}
	
	struct sockaddr_un address = { 0 };
	strcpy(address.sun_path, socketname);
	address.sun_family = AF_UNIX;
	
	DEBUG("connecting to %s\n", socketname);
	free(socketname);

	if (connect(sock, (struct sockaddr *)&address, sizeof(address)) != 0)
	{
		int saved_errno = errno;
		close(sock);
		return -saved_errno;
	}

	return sock;
}

int close_connection(int sock)
{
	DEBUG("closing connection for sock %d\n", sock);

	shutdown(sock, SHUT_RDWR);
	close(sock);

	return 0;
}

int send_command(int sock, const struct nss_command *cmd, const char *data)
{
	DEBUG("sending command (sock %d): %s\n", sock, describe_nss_command(cmd->command));
	if (send(sock, cmd, sizeof(*cmd), 0) != sizeof(*cmd))
	{
		return -1;
	}

	if (cmd->data_len > 0)
	{
		if (data == NULL)
		{
			return -1;
		}

		DEBUG("sending command data: %d bytes\n", cmd->data_len);

		return (send(sock, data, cmd->data_len, 0) == cmd->data_len ? 0 : -1);
	}

	return 0;
}

int recv_answer(int sock, struct nss_answer *ans, char **data)
{
	DEBUG("receiving answer (sock %d)\n", sock);
	if (recv(sock, ans, sizeof(*ans), 0) != sizeof(*ans))
	{
		return -EIO;
	}

	if (ans->data_len > 0)
	{
		DEBUG("receiving answer data (%u bytes)\n", ans->data_len);
		char *buffer = malloc(ans->data_len);
		if (buffer == NULL)
		{
			return -EIO;
		}

		if (recv(sock, buffer, ans->data_len, 0) != ans->data_len)
		{
			free(buffer);
			return -EIO;
		}

		if (data != NULL)
		{
			*data = buffer;
		}
		else
		{
			free(buffer);
		}
	}

	DEBUG("answer ret: %d, errno: %s\n", ans->ret, strerror(ans->ret_errno));
	return 0;
}

unsigned rfsnss_is_server_running(uid_t uid)
{
	int sock = make_connection(uid);
	
	if (sock <= 0)
	{
		return 0;
	}
	
	close_connection(sock);

	return 1;
}

