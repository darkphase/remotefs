/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <fcntl.h>
#include <unistd.h>

#include "buffer.h"
#include "config.h"
#include "instance_server.h"
#include "list.h"

static struct list* check_file_in_list(struct list *head, int file)
{
	struct list *item = head;
	while (item != NULL)
	{
		if (*((int *)(item->data)) == file)
		{
			return item;
		}
		
		item = item->next;
	}
	
	return NULL;
}

int add_file_to_list(struct list **head, int file)
{
	struct list *exist = check_file_in_list(*head, file);
	if (exist != NULL)
	{
		return 0;
	}

	int *handle = get_buffer(sizeof(file));
	if (handle == NULL)
	{
		return -1;
	}
	
	*handle = file;
	
	if (add_to_list(head, handle) == NULL)
	{
		free_buffer(handle);
		return -1;
	}
	
	return 0;
}

int add_file_to_open_list(struct rfsd_instance *instance, int file)
{
	DEBUG("adding file to open list: %d\n", file);
	return add_file_to_list(&instance->cleanup.open_files, file);
}

int remove_file_from_list(struct list **head, int file)
{
	struct list *exist = check_file_in_list(*head, file);
	if (exist == NULL)
	{
		return -1;
	}
	
	remove_from_list(head, exist);
	
	return 0;
}

int remove_file_from_open_list(struct rfsd_instance *instance, int file)
{
	DEBUG("removing file from open list: %d\n", file);
	return remove_file_from_list(&instance->cleanup.open_files, file);
}

int add_file_to_locked_list(struct rfsd_instance *instance, int file)
{
	DEBUG("adding file to locked list: %d\n", file);
	return add_file_to_list(&instance->cleanup.locked_files, file);
}

int remove_file_from_locked_list(struct rfsd_instance *instance, int file)
{
	DEBUG("removing file from locked list: %d\n", file);
	return remove_file_from_list(&instance->cleanup.locked_files, file);
}

int cleanup_files(struct rfsd_instance *instance)
{
	DEBUG("%s\n", "cleaninig up files");
	if (instance->cleanup.open_files != NULL)
	{
		struct list *item = instance->cleanup.open_files;
		while (item != 0)
		{
			DEBUG("closing still open handle: %d\n", *((int *)(item->data)));
			
			close(*((int *)(item->data)));
			item = item->next;
		}
		
		destroy_list(&instance->cleanup.open_files);
	}
	
	if (instance->cleanup.locked_files != NULL)
	{
		struct list *item = instance->cleanup.locked_files;
		while (item != 0)
		{
			DEBUG("unlocking still locked handle: %d\n", *((int *)(item->data)));
			
			struct flock fl = { 0 };
			fl.l_type = F_UNLCK;
			
			fcntl((*((int *)(item->data))), F_SETFL, &fl);
			
			item = item->next;
		}
		
		destroy_list(&instance->cleanup.locked_files);
	}

	return 0;
}

