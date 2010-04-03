/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
//#include <sys/stat.h>

#include "../buffer.h"
#include "../command.h"
#include "../config.h"
#include "../instance_client.h"
#include "../list.h"

#ifdef WITH_UGO

static int nss_answer(int sock, struct answer *ans, const char *data)
{
#ifdef RFS_DEBUG
	dump_answer(ans);
#endif

	if (send(sock, ans, sizeof(*ans), 0) != sizeof(*ans))
	{
		return -errno;
	}

	if (ans->data_len != 0 
	&& data != NULL)
	{
		if (send(sock, data, ans->data_len, 0) != ans->data_len)
		{
			return -errno;
		}
	}

	return 0;
}

static unsigned check_name(int sock, const struct list *names, struct command *cmd)
{
	char *name = malloc(cmd->data_len);

	if (recv(sock, name, cmd->data_len, 0) != cmd->data_len)
	{
		return errno;
	}

	if (strlen(name) != cmd->data_len - 1)
	{
		struct answer ans = { cmd->command, 0, -1, EINVAL };
		nss_answer(sock, &ans, NULL);
		return -EINVAL;
	}
	
	const struct list *entry = names;
	unsigned name_valid = 0;
	while (entry != NULL)
	{
		const char *entry_name = (const char *)entry->data;
	
		if (strcmp(entry_name, name) == 0)
		{
			name_valid = 1;
			break;
		}
		
		entry = entry->next;
	}
	
	DEBUG("name (%s) valid: %d\n", name, name_valid);
		
	free(name);
	
	struct answer ans = { cmd->command, 0, 0, (name_valid != 0 ? 0 : EINVAL) };
	int answer_ret = nss_answer(sock, &ans, NULL);

	return answer_ret;
}

static int process_checkuser(struct rfs_instance *instance, int sock, struct command *cmd)
{	
	DEBUG("%s\n", "processing checkuser");
	return check_name(sock, instance->nss.users_storage, cmd);
}

static int process_checkgroup(struct rfs_instance *instance, int sock, struct command *cmd)
{
	DEBUG("%s\n", "processing checkgroup");
	return check_name(sock, instance->nss.groups_storage, cmd);
}

static int send_names(int sock, const struct list *names, enum server_commands cmd_id)
{
	const struct list *name_entry = names;

	while (name_entry != NULL)
	{
		struct answer ans = { cmd_id, strlen(name_entry->data) + 1, 0, 0 };

		int answer_ret = nss_answer(sock, &ans, name_entry->data);
		if (answer_ret != 0)
		{
			return answer_ret;
		}
		
		name_entry = name_entry->next;
	}
	
	struct answer ans = { cmd_id, 0, 0, 0 };
	int answer_ret = nss_answer(sock, &ans, NULL);

	return answer_ret;
}

static int process_getusers(struct rfs_instance *instance, int sock, struct command *cmd)
{
	return send_names(sock, instance->nss.users_storage, (enum server_commands)cmd->command);
}

static int process_getgroups(struct rfs_instance *instance, int sock, struct command *cmd)
{
	return send_names(sock, instance->nss.groups_storage, (enum server_commands)cmd->command);
}

int process_command(struct rfs_instance *instance, int sock, struct command *cmd)
{
	switch (cmd->command)
	{
	case cmd_checkuser: 
		return process_checkuser(instance, sock, cmd);
	
	case cmd_checkgroup: 
		return process_checkgroup(instance, sock, cmd);

	case cmd_getusers:
		return process_getusers(instance, sock, cmd);
	
	case cmd_getgroups:
		return process_getgroups(instance, sock, cmd);

	default: 			
		{
		struct answer ans = { cmd->command, 0, -1, ENOTSUP };
		return nss_answer(sock, &ans, NULL);
		}
	}
}

#else
int nss_processing_c_empty_module = 0;
#endif /* WITH_UGO */
