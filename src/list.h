#ifndef LIST_H
#define LIST_H

/* linked list routines */

/** list node */
struct list
{
	struct list *prev;
	struct list *next;
	void *data;
};

/** add data to list. will NOT update head */
struct list* add_to_list(struct list *head, void *data);

/** remove node from list. will NOT update head */
struct list* remove_from_list(struct list *item);

/** delete whole list. will NOT update head */
void destroy_list(struct list *head);

#endif // LIST_H

