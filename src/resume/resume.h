/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef RESUME_H
#define RESUME_H

/** routines for file operations resuming by client */

#include <stdint.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct list;
struct flock;

/** record about open file */
struct open_rec
{
	char *path;
	int flags;
	uint64_t desc;
	size_t last_used_read_block;
};

/** record about locked file */
struct lock_rec
{
	char *path;
	int cmd;
	short type;
	short whence;
	off_t start;
	off_t len;
};

/** add file to list of open */
int resume_add_file_to_open_list(struct list **head, const char *path, int flags, uint64_t desc);

/** remove file from list of open */
int resume_remove_file_from_open_list(struct list **head, const char *path);

/** return file descriptor if file is recorder as open or -1 if not */
uint64_t resume_is_file_in_open_list(const struct list *head, const char *path);

/** remove file from list of locked */
int resume_remove_file_from_locked_list(struct list **head, const char *path);

/** clear lock info or add lock info for path depend on lock_cmd and */
int resume_update_file_lock_status(struct list **head, const char *path, int lock_cmd, struct flock *fl);

/** delete lists of open and locked files */
void destroy_resume_lists(struct list **open, struct list **locked);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* RESUME_H */

