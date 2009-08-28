/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include "config.h"

#include <list.h>
#include <stdlib.h>

void init_config(struct config *config)
{
	config->connections = 0;
	config->connections_updated = time(NULL);

	config->sock = -1;
	config->socketname = NULL;
	config->fork = 1;
	config->allow_other = 0;
	
	config->lock = -1;

	config->last_uid = (uid_t)(START_UID - 1);
	config->last_gid = (gid_t)(START_GID - 1);

	config->users = NULL;
	config->groups = NULL;

	config->user_cookies = NULL;
	config->group_cookies = NULL;

	config->maintenance_thread = 0;
	pthread_mutex_init(&config->maintenance_lock, NULL);
	config->stop_maintenance_thread = 0;
}

void release_config(struct config *config)
{
	release_users(&config->users);
	release_groups(&config->groups);

	pthread_mutex_destroy(&config->maintenance_lock);
}

void release_users(struct list **users)
{
	struct list *entry = *users;

	while (entry != NULL)
	{
		struct user_info *info = (struct user_info *)entry->data;

		free(info->name);
		
		entry = entry->next;
	}

	destroy_list(users);
}

void release_groups(struct list **groups)
{
	struct list *entry = *groups;

	while (entry != NULL)
	{
		struct group_info *info = (struct group_info *)entry->data;

		free(info->name);
		
		entry = entry->next;
	}

	destroy_list(groups);
}

