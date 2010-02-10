/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <stdlib.h>
#include <string.h>

#include "../buffer.h"
#include "../config.h"
#include "../command.h"
#include "../id_lookup.h"
#include "../instance_server.h"
#include "../list.h"
#include "../sendrecv.h"
#include "../server.h"

int _handle_getnames(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	/* send users */

	size_t users_len = 0;

	struct list *uid = instance->id_lookup.uids;
	while (uid != NULL)
	{
		struct uid_look_ent *entry = (struct uid_look_ent *)uid->data;
		users_len += strlen(entry->name) + 1;

		uid = uid->next;
	}

	struct answer ans = { cmd_getnames, users_len, 0, 0 };

	char *users = malloc(users_len);
	size_t written = 0;

	uid = instance->id_lookup.uids;
	while (uid != NULL)
	{	
		struct uid_look_ent *entry = (struct uid_look_ent *)uid->data;

		DEBUG("%s\n", entry->name);

		memcpy(users + written, entry->name, strlen(entry->name) + 1);
		written += strlen(entry->name) + 1;

		uid = uid->next;
	}

#ifdef RFS_DEBUG
	dump(users, users_len);
#endif

	send_token_t users_token = { 0, {{ 0 }} };
	if (do_send(&instance->sendrecv, 
		queue_data(users, users_len, 
		queue_ans(&ans, &users_token))) < 0)
	{
		free(users);
		return -1;
	}

	free(users);

	/* send groups */

	size_t groups_len = 0;

	struct list *gid = instance->id_lookup.gids;
	while (gid != NULL)
	{
		struct gid_look_ent *entry = (struct gid_look_ent *)gid->data;
		groups_len += strlen(entry->name) + 1;

		gid = gid->next;
	}

	ans.command = cmd_getnames;
	ans.data_len = groups_len;

	char *groups = malloc(groups_len);
	written = 0;

	gid = instance->id_lookup.gids;
	while (gid != NULL)
	{	
		struct gid_look_ent *entry = (struct gid_look_ent *)gid->data;

		memcpy(groups + written, entry->name, strlen(entry->name) + 1);
		written += strlen(entry->name) + 1;

		gid = gid->next;
	}
	
#ifdef RFS_DEBUG
	dump(groups, groups_len);
#endif

	send_token_t groups_token = { 0, {{ 0 }} };
	if (do_send(&instance->sendrecv, 
		queue_data(groups, groups_len, 
		queue_ans(&ans, &groups_token))) < 0)
	{
		free(groups);
		return -1;
	}

	free(groups);

	return 0;
}
