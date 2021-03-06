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

struct rfs_list;
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
	int lock_type;
	unsigned fully_locked;
};

/** add file to list of open 
\return 0 if successfully added 
\param flags file open flags 
\param desc descriptor of open file */
int resume_add_file_to_open_list(struct rfs_list **head, const char *path, int flags, uint64_t desc);

/** remove file from list of open 
\return 0 if successfully removed */
int resume_remove_file_from_open_list(struct rfs_list **head, const char *path);

/** check if file is recorded as open 
\return file descriptor if file is recorder as open or -1 if not */
uint64_t resume_is_file_in_open_list(const struct rfs_list *head, const char *path);

/** add file to list of locked 
\param fully_locked !0 if file is locked for full length
\return 0 on success */
int resume_add_file_to_locked_list(struct rfs_list **head, const char *path, int lock_type, unsigned fully_locked);

/** remove file from list of locked 
\return 0 if successfully removed */
int resume_remove_file_from_locked_list(struct rfs_list **head, const char *path);

/** check if file is recorded as locked 
\return !0 if file is recorder as locked */
unsigned resume_is_file_in_locked_list(const struct rfs_list *head, const char *path);

/** delete lists of open and locked files */
void destroy_resume_lists(struct rfs_list **open, struct rfs_list **locked);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* RESUME_H */

