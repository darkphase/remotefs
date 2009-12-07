/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include "../buffer.h"
#include "../config.h"
#include "../instance_client.h"
#include "../list.h"
#include "resume.h"

int resume_add_file_to_open_list(struct list **head, const char *path, int flags, uint64_t desc)
{
	DEBUG("adding %s to open list with %d flags\n", path, flags);
	struct list *item = *head;
	while (item != NULL)
	{
		struct open_rec *data = (struct open_rec *)item->data;
		if (strcmp(data->path, path) == 0)
		{
			data->flags = flags;
			return 0; /* it's already there */
		}
		item = item->next;
	}
	
	struct open_rec *new_item = get_buffer(sizeof(*new_item));
	if (new_item == NULL)
	{
		return -1;
	}
	
	new_item->path = strdup(path);
	new_item->flags = flags;
	new_item->desc = desc;
	new_item->last_used_read_block = 0;
	
	if (add_to_list(head, new_item) == NULL)
	{
		free(new_item->path);
		free_buffer(new_item);
		return -1;
	}
	
	return 0;
}

int resume_remove_file_from_open_list(struct list **head, const char *path)
{
	DEBUG("removing %s from open list\n", path);
	
	struct list *item = *head;
	while (item != NULL)
	{
		struct open_rec *data = (struct open_rec *)item->data;
		if (strcmp(data->path, path) == 0)
		{
			free(data->path);
			remove_from_list(head, item);

			return 0;
		}
		item = item->next;
	}

	return 0;
}

uint64_t resume_is_file_in_open_list(const struct list *head, const char *path)
{
	const struct list *item = head;
	while (item != NULL)
	{
		struct open_rec *data = (struct open_rec *)item->data;
		if (strcmp(data->path, path) == 0)
		{
			return data->desc;
		}
		item = item->next;
	}

	return -1;
}

int resume_add_file_to_locked_list(struct list **head, const char *path, int lock_type, unsigned fully_locked)
{
	DEBUG("adding %s to locked list %s\n", path, fully_locked ? "(fully locked)" : "");

	struct list *item = *head;
	while (item != NULL)
	{
		struct lock_rec *data = (struct lock_rec *)(item->data);
		if (strcmp(data->path, path) == 0)
		{
			data->lock_type = lock_type;
			data->fully_locked = fully_locked;
			return 0;
		}

		item = item->next;
	}

	struct lock_rec *new_item = get_buffer(sizeof(*new_item));
	if (new_item == NULL)
	{
		return -1;
	}

	new_item->path = strdup(path);
	new_item->lock_type = lock_type;
	new_item->fully_locked = fully_locked;

	if (add_to_list(head, new_item) == NULL)
	{
		free(new_item->path);
		free_buffer(new_item);
		return -1;
	}

	return 0;
}

int resume_remove_file_from_locked_list(struct list **head, const char *path)
{
	DEBUG("removing %s from locked list\n", path);
	struct list *item = *head;
	while (item != NULL)
	{
		struct lock_rec *data = (struct lock_rec *)item->data;
		if (strcmp(data->path, path) == 0)
		{
			free(data->path);
			item = remove_from_list(head, item);

			continue;
		}
		item = item->next;
	}
	
	return 0;
}

void destroy_resume_lists(struct list **open, struct list **locked)
{
	DEBUG("%s\n", "destroying resume lists");
	struct list *open_item = *open;
	while (open_item != NULL)
	{
		free(((struct open_rec *)open_item->data)->path);
		open_item = open_item->next;
	}
	destroy_list(open);
	*open = NULL;
	
	struct list *lock_item = *locked;
	while (lock_item != NULL)
	{
		free(((struct lock_rec *)lock_item->data)->path);
		lock_item = lock_item->next;
	}
	destroy_list(locked);
	*locked = NULL;
}

