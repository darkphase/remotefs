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

struct rfsd_instance;

/** add descriptor to list of open files */
int add_file_to_open_list(struct rfsd_instance *instance, int file);

/** remove descriptor from list of open files */
int remove_file_from_open_list(struct rfsd_instance *instance, int file);

/** unlock locked files ans close open ones */
int cleanup_files(struct rfsd_instance *instance);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* CLEANUP_H */

