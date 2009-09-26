/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef LIST_H
#define LIST_H

/** linked list routines */

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/** list node */
struct list
{
	struct list *prev;
	struct list *next;
	void *data;
};

/** add data to list */
struct list* add_to_list(struct list **head, void *data);

/** remove node from list 
\return next item*/
struct list* remove_from_list(struct list **head, struct list *item);

/** remove node from list, but not delete associated data */
void* extract_from_list(struct list **head, struct list *item);

/** delete whole list */
void destroy_list(struct list **head);

/** *count* list length */
unsigned list_length(const struct list *head);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* LIST_H */

