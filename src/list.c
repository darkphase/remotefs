#include "list.h"

#include "config.h"
#include "buffer.h"

struct list* add_to_list(struct list *head, void *data)
{
	while (head != NULL && head->next != NULL)
	{
		head = head->next;
	}

	struct list *new_item = get_buffer(sizeof(*head));
	if (new_item == NULL)
	{
		return NULL;
	}

	new_item->prev = head;
	new_item->next = NULL;
	new_item->data = data;

	if (head != NULL)
	{
		head->next = new_item;
	}
	
	return new_item;
}

struct list* remove_from_list(struct list *item)
{
	if (item->prev != NULL)
	{
		item->prev->next = item->next;
	}

	if (item->next != NULL)
	{
		item->next->prev = item->prev;
	}

	struct list *ret = item->next;

	free_buffer(item->data);
	free_buffer(item);

	return ret;
}

void destroy_list(struct list *head)
{
	if (head == NULL)
	{
		return;
	}

	if (head->prev)
	{
		head->prev->next = NULL;
	}

	while (head != NULL)
	{
		struct list *next = head->next;

		free_buffer(head->data);
		free_buffer(head);

		head = next;
	}
}

