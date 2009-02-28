/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include "buffer.h"
#include "config.h"
#include "list.h"

struct list* add_to_list(struct list **head, void *data)
{
	struct list *tail = *head;
	while (tail != NULL && tail->next != NULL)
	{
		tail = tail->next;
	}

	struct list *new_item = get_buffer(sizeof(*new_item));
	if (new_item == NULL)
	{
		return NULL;
	}

	new_item->prev = tail;
	new_item->next = NULL;
	new_item->data = data;

	if (tail != NULL)
	{
		tail->next = new_item;
	}
	
	if (*head == NULL)
	{
		*head = new_item;
	}
	
	return new_item;
}

struct list* remove_from_list(struct list **head, struct list *item)
{
	if (item == *head)
	{
		*head = item->next;
	}

	if (item->prev != NULL)
	{
		item->prev->next = item->next;
	}

	if (item->next != NULL)
	{
		item->next->prev = item->prev;
	}

	struct list *ret = item->next;
	
	if (item == *head)
	{
		*head = ret;
	}

	free_buffer(item->data);
	free_buffer(item);

	return ret;
}

void destroy_list(struct list **head)
{
	if (*head == NULL)
	{
		return;
	}

	if ((*head)->prev)
	{
		(*head)->prev->next = NULL;
	}

	struct list *item = *head;
	while (item != NULL)
	{
		struct list *next = item->next;
		
		free_buffer(item->data);
		free_buffer(item);
		
		item = next;
	}
	
	*head = NULL;
}

