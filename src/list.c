/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <stdlib.h>

#include "buffer.h"
#include "config.h"
#include "list.h"

struct rfs_list* add_to_list(struct rfs_list **head, void *data)
{
	struct rfs_list *tail = *head;
	while (tail != NULL && tail->next != NULL)
	{
		tail = tail->next;
	}

	struct rfs_list *new_item = malloc(sizeof(*new_item));
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

void* extract_from_list(struct rfs_list **head, struct rfs_list *item)
{
	if (item->prev != NULL)
	{
		item->prev->next = item->next;
	}

	if (item->next != NULL)
	{
		item->next->prev = item->prev;
	}

	struct rfs_list *next = item->next;
	
	if (item == *head)
	{
		*head = next;
	}

	void *data = item->data;

	free(item);

	return data;
}

struct rfs_list* remove_from_list(struct rfs_list **head, struct rfs_list *item)
{
	struct rfs_list *next = item->next;

	void *data = extract_from_list(head, item);
	
	free(data);

	return next;
}

void destroy_list(struct rfs_list **head)
{
	if (*head == NULL)
	{
		return;
	}

	if ((*head)->prev)
	{
		(*head)->prev->next = NULL;
	}

	struct rfs_list *item = *head;
	while (item != NULL)
	{
		struct rfs_list *next = item->next;
		
		free(item->data);
		free(item);
		
		item = next;
	}
	
	*head = NULL;
}

unsigned list_length(const struct rfs_list *head)
{
	size_t count = 0;

	const struct rfs_list *item = head;
	while (item != NULL)
	{
		++count;
		item = item->next;
	}

	return count;
}
