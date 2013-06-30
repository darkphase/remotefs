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

#include "client.h"
#include "client_common.h"
#include "config.h"
#include "nss_cmd.h"

static struct passwd pwret = { 0 };
static struct group grret = { 0 };

static uint32_t get_uint32(uid_t self_uid, enum nss_commands nss_cmd, const char *name)
{
	int sock = make_connection(self_uid);
	
	if (sock <= 0)
	{
		return (uint32_t)-1;
	}
	
	size_t overall_size = strlen(name) + 1;

	struct nss_command cmd = { nss_cmd, overall_size };

	if (send_command(sock, &cmd, name) < 0)
	{
		close_connection(sock);
		return (uint32_t)-1;
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
		return (uint32_t)-1;
	}
		
	close_connection(sock);
	
	uint32_t id32 = (uint32_t)-1;

	if (buffer == NULL 
	|| ans.command != nss_cmd 
	|| ans.ret < 0 
	|| ans.data_len != sizeof(id32))
	{
		free(buffer);
		return (uint32_t)-1;
	}

	id32 = *(uint32_t *)buffer;

	free(buffer);

	return id32;
}

static char* get_string(uid_t self_uid, enum nss_commands nss_cmd, uint32_t id32)
{
	int sock = make_connection(self_uid);
	
	if (sock <= 0)
	{
		return NULL;
	}

	struct nss_command cmd = { nss_cmd, sizeof(id32) };

	if (send_command(sock, &cmd, (char *)&id32) < 0)
	{
		close_connection(sock);
		return NULL;
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
		return NULL;
	}

	if (ans.command != nss_cmd
	|| ans.ret < 0
	|| buffer == NULL)
	{
		if (buffer != NULL)
		{
			free(buffer);
		}

		close_connection(sock);
		return NULL;
	}

	return buffer;
}

static struct passwd* _rfsnss_getpwnam(uid_t self_uid,const char *name)
{
	int saved_errno = errno;

	uint32_t uid32 = get_uint32(self_uid, cmd_getpwnam, name);
	if (uid32 == (uint32_t)-1)
	{
		return NULL;
	}

	if (pwret.pw_name != NULL)
	{
		free(pwret.pw_name);
	}
	
	memset(&pwret, 0, sizeof(pwret));
	
	pwret.pw_name   = strdup(name);
	pwret.pw_uid    = (uid_t)(uid32);
	pwret.pw_gid    = (gid_t)-1;

	errno = saved_errno;
	return &pwret;
}

static struct passwd* _rfsnss_getpwuid(uid_t self_uid, uid_t uid)
{
	int saved_errno = errno;

	char *name = get_string(self_uid, cmd_getpwuid, (uint32_t)uid);

	if (name == NULL)
	{
		return NULL;
	}
	
	if (pwret.pw_name != NULL)
	{
		free(pwret.pw_name);
	}

	memset(&pwret, 0, sizeof(pwret));
	
	pwret.pw_name   = name;
	pwret.pw_uid    = uid;
	pwret.pw_gid    = (gid_t)-1;

	errno = saved_errno;
	return &pwret;
}

static struct group* _rfsnss_getgrnam(uid_t self_uid, const char *name)
{
	int saved_errno = errno;

	uint32_t gid32 = get_uint32(self_uid, cmd_getgrnam, name);
	if (gid32 == (uint32_t)-1)
	{
		return NULL;
	}
	
	memset(&grret, 0, sizeof(grret));

	if (grret.gr_name != NULL)
	{
		free(grret.gr_name);
	}

	grret.gr_name   = strdup(name);
	grret.gr_gid    = (gid_t)gid32;

	errno = saved_errno;
	return &grret;
}

static struct group* _rfsnss_getgrgid(uid_t self_uid, gid_t gid)
{
	int saved_errno = errno;

	char *name = get_string(self_uid, cmd_getgrgid, (uint32_t)gid);
	
	if (name == NULL)
	{
		return NULL;
	}

	if (grret.gr_name != NULL)
	{
		free(grret.gr_name);
	}

	memset(&grret, 0, sizeof(grret));
	
	grret.gr_name   = name;
	grret.gr_gid    = gid;

	errno = saved_errno;
	return &grret;
}

struct passwd* rfsnss_getpwnam(const char *name)
{
	CHECK_BOTH_SERVERS(_rfsnss_getpwnam, name);
}

struct passwd* rfsnss_getpwuid(uid_t uid)
{
	CHECK_BOTH_SERVERS(_rfsnss_getpwuid, uid);
}

struct group* rfsnss_getgrnam(const char *name)
{
	CHECK_BOTH_SERVERS(_rfsnss_getgrnam, name);
}

struct group* rfsnss_getgrgid(gid_t gid)
{
	CHECK_BOTH_SERVERS(_rfsnss_getgrgid, gid);
}

