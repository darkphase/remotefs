/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "resume.h"
#include "list.h"
#include "buffer.h"
#include "instance.h"

int add_file_to_open_list(struct rfs_instance *instance, const char *path, int flags, uint64_t desc)
{
	DEBUG("adding %s to open list with %d flags\n", path, flags);
	struct list *item = instance->resume.open_files;
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
	
	if (add_to_list(&instance->resume.open_files, new_item) == NULL)
	{
		free(new_item->path);
		free_buffer(new_item);
		return -1;
	}
	
	return 0;
}

int remove_file_from_open_list(struct rfs_instance *instance, const char *path)
{
	DEBUG("removing %s from open list\n", path);
	
	struct list *item = instance->resume.open_files;
	while (item != NULL)
	{
		struct open_rec *data = (struct open_rec *)item->data;
		if (strcmp(data->path, path) == 0)
		{
			free(data->path);
			remove_from_list(&instance->resume.open_files, item);
			return 0;
		}
		item = item->next;
	}
	
	return 0;
}

int add_file_to_locked_list(struct rfs_instance *instance, const char *path, int cmd, short type, short whence, off_t start, off_t len)
{
	DEBUG("adding %s to locked list\n", path);
	struct list *item = instance->resume.locked_files;
	while (item != NULL)
	{
		struct lock_rec *data = (struct lock_rec *)item->data;
		if (strcmp(data->path, path) == 0)
		{
			data->cmd = cmd;
			data->type = type;
			data->whence = whence;
			data->start = start;
			data->len = len;
			return 0; /* it's alread there */
		}
		item = item->next;
	}
	
	struct lock_rec *new_item = get_buffer(sizeof(*new_item));
	if (new_item == NULL)
	{
		return -1;
	}
	
	new_item->path = strdup(path);
	new_item->cmd = cmd;
	new_item->type = type;
	new_item->whence = whence;
	new_item->start = start;
	new_item->len = len;
	
	if (add_to_list(&instance->resume.locked_files, new_item) == NULL)
	{
		free(new_item->path);
		free_buffer(new_item);
		return -1;
	}
	
	return 0;
}

int remove_file_from_locked_list(struct rfs_instance *instance, const char *path)
{
	DEBUG("removing %s from locked list\n", path);
	struct list *item = instance->resume.locked_files;
	while (item != NULL)
	{
		struct lock_rec *data = (struct lock_rec *)item->data;
		if (strcmp(data->path, path) == 0)
		{
			free(data->path);
			remove_from_list(&instance->resume.locked_files, item);
			
			return 0;
		}
		item = item->next;
	}
	
	return 0;
}

const struct lock_rec* get_lock_info(struct rfs_instance *instance, const char *path)
{
	struct list *item = instance->resume.locked_files;
	while (item != NULL)
	{
		struct lock_rec *data = (struct lock_rec *)item->data;
		if (strcmp(data->path, path) == 0)
		{
			return data;
		}
		item = item->next;
	}
	
	return NULL;
}

void destroy_resume_lists(struct rfs_instance *instance)
{
	DEBUG("%s\n", "destroying resume lists");
	struct list *open_item = instance->resume.open_files;
	while (open_item != NULL)
	{
		free(((struct open_rec *)open_item->data)->path);
		open_item = open_item->next;
	}
	destroy_list(&instance->resume.open_files);
	instance->resume.open_files = NULL;
	
	struct list *lock_item = instance->resume.locked_files;
	while (lock_item != NULL)
	{
		free(((struct lock_rec *)lock_item->data)->path);
		lock_item = lock_item->next;
	}
	destroy_list(&instance->resume.locked_files);
	instance->resume.locked_files = NULL;
}
