/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "../buffer.h"
#include "../config.h"
#include "../instance_server.h"
#include "../list.h"
#include "cleanup.h"

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

static int add_file_to_list(struct list **head, int file)
{
	struct list *exist = check_file_in_list(*head, file);
	if (exist != NULL)
	{
		return 0;
	}

	int *handle = malloc(sizeof(file));
	if (handle == NULL)
	{
		return -1;
	}
	
	*handle = file;
	
	if (add_to_list(head, handle) == NULL)
	{
		free(handle);
		return -1;
	}
	
	return 0;
}

int cleanup_add_file_to_open_list(struct list **head, int file)
{
	DEBUG("adding file to open list: %d\n", file);
	return add_file_to_list(head, file);
}

static int remove_file_from_list(struct list **head, int file)
{
	struct list *exist = check_file_in_list(*head, file);
	if (exist == NULL)
	{
		return -1;
	}
	
	remove_from_list(head, exist);
	
	return 0;
}

int cleanup_remove_file_from_open_list(struct list **head, int file)
{
	DEBUG("removing file from open list: %d\n", file);
	return remove_file_from_list(head, file);
}

int cleanup_files(struct list **open)
{
	DEBUG("%s\n", "cleaninig up files");

	if (*open != NULL)
	{
		struct list *item = *open;
		while (item != 0)
		{
			DEBUG("closing still open handle: %d\n", *((int *)(item->data)));
			
			close(*((int *)(item->data)));
			item = item->next;
		}
		
		destroy_list(open);
		*open = NULL;
	}
	
	return 0;
}

