/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "cookies.h"
#include "nss_cmd.h"
#include "processing_common.h"
#include "processing_ent.h"

#include <list.h>

static int process_marker(int sock, const struct nss_command *cmd, struct config *config, enum nss_commands nss_cmd)
{
	char *buffer = NULL;

	if (recv_command_data(sock, cmd, &buffer) < 0)
	{
		return -1;
	}

	if (cmd->command != nss_cmd
	|| cmd->data_len != sizeof(uint32_t))
	{
		if (buffer != NULL)
		{
			free(buffer);
		}
		return reject_request(sock, cmd, EINVAL);
	}

	pid_t pid = (pid_t)(*(uint32_t *)buffer);

	free(buffer);

	DEBUG("received pid: %u\n", pid);

	void **cookies = NULL;

	switch (nss_cmd)
	{
		case cmd_setpwent:
		case cmd_endpwent:
			cookies = &config->user_cookies;
			break;
		case cmd_setgrent:
		case cmd_endgrent:
			cookies = &config->group_cookies;
			break;
		default:
			return reject_request(sock, cmd, ECANCELED);
	}

	switch (nss_cmd)
	{
		case cmd_setpwent:
		case cmd_setgrent:
		{	
			struct cookie *cookie = NULL;

			if (cookies != NULL 
			&& *cookies != NULL)
			{
				cookie = get_cookie(cookies, pid);
			}

			if (cookie != NULL)
			{
				cookie->value = 0;
			}
			else
			{
				cookie = create_cookie(cookies, pid);

				DEBUG("%s\n", "new cookie:");
#ifdef RFS_DEBUG
				dump_cookie(cookie);
#endif

				if (cookie == NULL)
				{
					return reject_request(sock, cmd, ECANCELED);
				}
			}

			break;
		}

		case cmd_endpwent:
		case cmd_endgrent:
			delete_cookie(cookies, pid);
			break;

		default:	
			return reject_request(sock, cmd, ECANCELED);
	}

	struct nss_answer ans = { nss_cmd, 0, 0, 0 };

	if (send_answer(sock, &ans, NULL) < 0)
	{
		return -1;
	}

	return 0;
}

static int process_getent(int sock, const struct nss_command *cmd, struct config *config, enum nss_commands nss_cmd)
{
	char *buffer = NULL;

	if (recv_command_data(sock, cmd, &buffer) < 0)
	{
		return -1;
	}

	if (cmd->command != nss_cmd
	|| cmd->data_len != sizeof(uint32_t))
	{
		if (buffer != NULL)
		{
			free(buffer);
		}
		return reject_request(sock, cmd, EINVAL);
	}

	pid_t pid = (pid_t)(*(uint32_t *)buffer);

	free(buffer);
	
	DEBUG("received pid: %u\n", pid);

	struct cookie *ent_cookie = NULL;

	switch (nss_cmd)
	{
		case cmd_getpwent:
			ent_cookie = get_cookie(&config->user_cookies, pid);
			break;
		case cmd_getgrent:
			ent_cookie = get_cookie(&config->group_cookies, pid);
			break;
		default:
			return reject_request(sock, cmd, ECANCELED);
	}

	if (ent_cookie == NULL)
	{
		DEBUG("%s\n", "cookie not found: creating it");

		switch (nss_cmd)
		{
			case cmd_getpwent:
				ent_cookie = create_cookie(&config->user_cookies, pid);
				break;
			case cmd_getgrent:
				ent_cookie = create_cookie(&config->group_cookies, pid);
				break;
			default:
				break;
		}
	}

	if (ent_cookie == NULL)
	{
		return reject_request(sock, cmd, ECANCELED);
	}

	DEBUG("current cookie value for this pid: %u\n", ent_cookie->value);

	struct list *item = NULL;

	switch (nss_cmd)
	{
		case cmd_getpwent:
			item = config->users;
			break;
		case cmd_getgrent:
			item = config->groups;
			break;
		default:
			break;
	}

	int i = 0; for (i = 0; i < ent_cookie->value; ++i)
	{
		item = item->next;

		if (item == NULL)
		{
			return reject_request(sock, cmd, 0); /* reject request with errno == 0 to notify client about list end */
		}
	}

	if (item == NULL)
	{
		return reject_request(sock, cmd, ENOENT);
	}

	uint32_t id32 = (uint32_t)-1;
	const char *name = NULL;
	
	switch (nss_cmd)
	{
		case cmd_getpwent:
			id32 = (uint32_t)((struct user_info *)item->data)->uid;
			name = ((struct user_info *)item->data)->name;
			break;
		case cmd_getgrent:
			id32 = (uint32_t)((struct group_info *)item->data)->gid;
			name = ((struct group_info *)item->data)->name;
			break;
		default:
			break;
	}
	
	DEBUG("sending name %s and id %u\n", name, id32);

	size_t overall_len = sizeof(id32) + strlen(name) + 1;
	char *ans_buffer = malloc(overall_len);

	memcpy(ans_buffer, &id32, sizeof(id32));
	memcpy(ans_buffer + sizeof(id32), name, strlen(name) + 1); /* copy \0 too */
	
	struct nss_answer ans = { nss_cmd, overall_len, 0, 0 };

	if (send_answer(sock, &ans, ans_buffer) < 0)
	{
		free(ans_buffer);
		return -1;
	}

	free(ans_buffer);

	++ent_cookie->value;
	
	DEBUG("new cookie value for this pid: %u\n", ent_cookie->value);

	return 0;
}

int process_getpwent(int sock, const struct nss_command *cmd, struct config *config)
{
	return process_getent(sock, cmd, config, cmd_getpwent);
}

int process_setpwent(int sock, const struct nss_command *cmd, struct config *config)
{
	return process_marker(sock, cmd, config, cmd_setpwent);
}

int process_endpwent(int sock, const struct nss_command *cmd, struct config *config)
{
	return process_marker(sock, cmd, config, cmd_endpwent);
}

int process_getgrent(int sock, const struct nss_command *cmd, struct config *config)
{
	return process_getent(sock, cmd, config, cmd_getgrent);
}

int process_setgrent(int sock, const struct nss_command *cmd, struct config *config)
{
	return process_marker(sock, cmd, config, cmd_setgrent);
}

int process_endgrent(int sock, const struct nss_command *cmd, struct config *config)
{
	return process_marker(sock, cmd, config, cmd_endgrent);
}

