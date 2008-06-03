#ifndef LIST_H
#define LIST_H

struct list
{
	struct list *prev;
	struct list *next;
	void *data;
};

struct list* add_to_list(struct list *head, void *data);
struct list* remove_from_list(struct list *item);
void destroy_list(struct list *head);

#endif // LIST_H

