/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "client_common.h"
#include "client_for_server.h"
#include "config.h"
#include "nss_cmd.h"

int rfsnss_addserver(uid_t uid, const char *rfs_server)
{
	int sock = make_connection(uid);
	
	if (sock <= 0)
	{
		return -EIO;
	}
	
	struct nss_command cmd = { cmd_addserver, strlen(rfs_server) + 1 };
	if (send_command(sock, &cmd, rfs_server) < 0)
	{
		close_connection(sock);
		return -EIO;
	}

	struct nss_answer ans = { 0 };

	if (recv_answer(sock, &ans, NULL) < 0)
	{
		close_connection(sock);
		return -EIO;
	}

	if (ans.command != cmd_addserver
	|| ans.data_len > 0)
	{
		close_connection(sock);
		return -EINVAL;
	}
	
	close_connection(sock);

	if (ans.ret != 0)
	{
		return -ans.ret_errno;
	}
	
	int inc_sock = make_connection(uid);
	
	if (inc_sock <= 0)
	{
		return -EIO;
	}

	struct nss_command inc_cmd = { cmd_inc, 0 };
	if (send_command(inc_sock, &inc_cmd, NULL) < 0)
	{
		close_connection(inc_sock);
		return -EIO;
	}
		
	close_connection(inc_sock);

	return 0;
}

int rfsnss_dec(uid_t uid)
{
	int sock = make_connection(uid);
	
	if (sock <= 0)
	{
		return -EIO;
	}

	struct nss_command cmd = { cmd_dec, 0 };
	int ret = send_command(sock, &cmd, NULL);

	close_connection(sock);

	return (ret == 0 ? 0 : -EIO);
}

int rfsnss_adduser(const char *username, uid_t user_uid, uid_t uid)
{
	int sock = make_connection(uid);
	
	if (sock <= 0)
	{
		return -EIO;
	}
	
	uint32_t user_uid32 = user_uid;

	size_t overall_size = sizeof(user_uid32) + strlen(username) + 1;
	char *buffer = malloc(overall_size);

	if (buffer == NULL)
	{
		close_connection(sock);
		return -EIO;
	}

	memcpy(buffer, &user_uid32, sizeof(user_uid32));
	memcpy(buffer + sizeof(user_uid32), username, strlen(username));
	*(buffer + sizeof(user_uid32) + strlen(username)) = 0;

	struct nss_command cmd = { cmd_adduser, overall_size };
	if (send_command(sock, &cmd, buffer) < 0)
	{
		free(buffer);
		close_connection(sock);
		return -EIO;
	}
		
	free(buffer);
	
	struct nss_answer ans = { 0 };

	if (recv_answer(sock, &ans, NULL) < 0)
	{
		close_connection(sock);
		return -EIO;
	}

	if (ans.command != cmd_adduser
	|| ans.data_len > 0)
	{
		close_connection(sock);
		return -EINVAL;
	}

	close_connection(sock);

	return (ans.ret == 0 ? ans.ret : -ans.ret_errno);
}

int rfsnss_addgroup(const char *groupname, gid_t group_gid, uid_t uid)
{
	int sock = make_connection(uid);
	
	if (sock <= 0)
	{
		return -EIO;
	}
	
	uint32_t group_gid32 = group_gid;

	size_t overall_size = sizeof(group_gid32) + strlen(groupname) + 1;
	char *buffer = malloc(overall_size);

	if (buffer == NULL)
	{
		close_connection(sock);
		return -EIO;
	}

	memcpy(buffer, &group_gid32, sizeof(group_gid32));
	memcpy(buffer + sizeof(group_gid32), groupname, strlen(groupname));
	*(buffer + sizeof(group_gid32) + strlen(groupname)) = 0;

	struct nss_command cmd = { cmd_addgroup, overall_size };
	if (send_command(sock, &cmd, buffer) < 0)
	{
		free(buffer);
		close_connection(sock);
		return -EIO;
	}
		
	free(buffer);
	
	struct nss_answer ans = { 0 };

	if (recv_answer(sock, &ans, NULL) < 0)
	{
		close_connection(sock);
		return -EIO;
	}

	if (ans.command != cmd_addgroup 
	|| ans.data_len > 0)
	{
		close_connection(sock);
		return -EINVAL;
	}

	close_connection(sock);

	return (ans.ret == 0 ? ans.ret : -ans.ret_errno);
}

