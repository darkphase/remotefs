/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef RESUME_H
#define RESUME_H

/** routines for file operations resuming by client */

#include <sys/types.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct list;
struct rfs_instance;

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
int add_file_to_open_list(struct rfs_instance *instance, const char *path, int flags, uint64_t desc);

/** remove file from list of open */
int remove_file_from_open_list(struct rfs_instance *instance, const char *path);

/** add file to list of locked */
int add_file_to_locked_list(struct rfs_instance *instance, const char *path, int cmd, short type, short whence, off_t start, off_t len);

/** remove file from list of locked */
int remove_file_from_locked_list(struct rfs_instance *instance, const char *path);

/** get list of locked files */
const struct lock_rec* get_lock_info(struct rfs_instance *instance, const char *path);

/** delete lists of open and locked files */
void destroy_resume_lists(struct rfs_instance *instance);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* RESUME_H */
