/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef CLEANUP_H
#define CLEANUP_H

/** routines for files cleaning up on server after disconnect */

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct list;

/** add descriptor to list of open files 
\return 0 on success */
int cleanup_add_file_to_open_list(struct list **head, int file);

/** remove descriptor from list of open files 
\return 0 on success */
int cleanup_remove_file_from_open_list(struct list **head, int file);

/** unlock locked files ans close open ones 
\return 0 on success */
int cleanup_files(struct list **open);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* CLEANUP_H */

