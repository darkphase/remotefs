/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "client_common.h"
#include "client_ent.h"
#include "config.h"
#include "nss_cmd.h"

static struct passwd pwret = { 0 };
static struct group grret = { 0 };

static int set_marker(uid_t uid, enum nss_commands nss_cmd)
{
	int sock = make_connection(uid);

	if (sock <= 0)
	{
		return -EAGAIN;
	}
	
	uint32_t pid32 = (uint32_t)getpid();

	struct nss_command cmd = { nss_cmd, sizeof(pid32) };

	if (send_command(sock, &cmd, (char *)&pid32) < 0)
	{
		close_connection(sock);
		return -ECONNABORTED;
	}

	struct nss_answer ans = { 0 };

	if (recv_answer(sock, &ans, NULL) < 0)
	{
		close_connection(sock);
		return -ECONNABORTED;
	}

	if (ans.command != nss_cmd
	|| ans.data_len > 0)
	{
		close_connection(sock);
		return -EIO;
	}

	close_connection(sock);

	return ans.ret_errno;
}

static int get_id_and_name(uid_t uid, enum nss_commands nss_cmd, uint32_t *id32, char **name)
{
	int sock = make_connection(uid);

	if (sock <= 0)
	{
		return -EAGAIN;
	}

	uint32_t pid32 = (uint32_t)getpid();

	struct nss_command cmd = { nss_cmd, sizeof(pid32) };

	if (send_command(sock, &cmd, (char *)&pid32) < 0)
	{
		close_connection(sock);
		return -EIO;
	}

	struct nss_answer ans = { 0 };

	char *buffer = NULL;

	if (recv_answer(sock, &ans, &buffer) < 0)
	{
		if (buffer != NULL)
		{
			free(buffer);
		}

		close_connection(sock);
		return -EIO;
	}

	*id32 = (uint32_t)-1;
	*name = NULL;

	if (ans.command != nss_cmd
	|| ans.data_len < sizeof(*id32) + 2) /* 2 means some (at least 1-character) name and ending \0 */
	{
		if (buffer != NULL)
		{
			free(buffer);
		}

		close_connection(sock);
		return -EIO;
	}

	*id32 = *(uint32_t *)buffer;
	*name = strdup(buffer + sizeof(*id32));

	DEBUG("received name %s and id %u\n", *name, *id32);

	free(buffer);

	close_connection(sock);	

	return 0;
}

static struct passwd* _rfsnss_getpwent(uid_t uid)
{
	uint32_t uid32 = (uint32_t)-1;
	char *name = NULL;
	
	if (get_id_and_name(uid, cmd_getpwent, &uid32, &name) != 0)
	{
		return NULL;
	}
	
	if (uid32 == (uint32_t)-1
	&& name == NULL)
	{
		return NULL;
	}

	if (pwret.pw_name != NULL)
	{
		free(pwret.pw_name);
	}

	memset(&pwret, 0, sizeof(pwret));

	pwret.pw_name = name;
	pwret.pw_uid = (uid_t)uid32;
	pwret.pw_gid = (gid_t)-1;
	
	return &pwret;
}

static void _rfsnss_setpwent(uid_t uid)
{
	set_marker(uid, cmd_setpwent);
}

static void _rfsnss_endpwent(uid_t uid)
{
	set_marker(uid, cmd_endpwent);
}

static struct group* _rfsnss_getgrent(uid_t uid)
{
	uint32_t gid32 = (uint32_t)-1;
	char *name = NULL;
	
	if (get_id_and_name(uid, cmd_getgrent, &gid32, &name) != 0)
	{
		return NULL;
	}
	
	if (gid32 == (uint32_t)-1
	&& name == NULL)
	{
		return NULL;
	}

	if (grret.gr_name != NULL)
	{
		free(grret.gr_name);
	}

	memset(&grret, 0, sizeof(grret));

	grret.gr_name = name;
	grret.gr_gid = (gid_t)gid32;

	return &grret;
}

static void _rfsnss_setgrent(uid_t uid)
{
	set_marker(uid, cmd_setgrent);
}

static void _rfsnss_endgrent(uid_t uid)
{
	set_marker(uid, cmd_endgrent);
}

struct passwd* rfsnss_getpwent(void)
{
	CHECK_BOTH_SERVERS_VOID(_rfsnss_getpwent);
}

void rfsnss_setpwent(void)
{
	APPLY_TO_BOTH_SERVERS(_rfsnss_setpwent);
}

void rfsnss_endpwent(void)
{
	APPLY_TO_BOTH_SERVERS(_rfsnss_endpwent);
}

struct group* rfsnss_getgrent(void)
{
	CHECK_BOTH_SERVERS_VOID(_rfsnss_getgrent);
}

void rfsnss_setgrent(void)
{
	APPLY_TO_BOTH_SERVERS(_rfsnss_setgrent);
}

void rfsnss_endgrent(void)
{
	APPLY_TO_BOTH_SERVERS(_rfsnss_endgrent);
}

