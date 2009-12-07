/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#ifdef SOLARIS
#	include <fcntl.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../operations.h"
#include "../config.h"
#include "../instance_client.h"
#include "../list.h"
#include "client.h"
#include "resume.h"

int resume_files(struct rfs_instance *instance)
{
	/* we're doing this inside of maintenance() call (or inside of rfs_reconnect()), 
	so keep alive is already locked */
	
	DEBUG("%s\n", "beginning to resume connection");
	
	int ret = 0; /* last error */
	unsigned resume_failed = 0;
	
	/* reopen files */
	const struct list *open_file = instance->resume.open_files;
	while (open_file != NULL)
	{
		struct open_rec *data = (struct open_rec *)open_file->data;
		uint64_t desc = (uint64_t)-1;
		uint64_t prev_desc = data->desc;
		
		DEBUG("reopening file %s\n", data->path);

		int open_ret = _rfs_open(instance, data->path, data->flags, &desc);
		if (open_ret < 0)
		{
			ret = open_ret;
			resume_remove_file_from_open_list(&instance->resume.open_files, data->path);
			resume_remove_file_from_locked_list(&instance->resume.locked_files, data->path);
			
			/* if even single file wasn't reopened, then
			force whole operation fail to prevent descriptors
			mixing and stuff */
			
			resume_failed = 1;
			break;
		}
		
		if (desc != prev_desc)
		{
			/* nope, we're not satisfied with another descriptor 
			those descriptors will be used in read() and write() so 
			files should be opened exactly with the same descriptors */
			
			resume_failed = 1;
			break;
		}

		DEBUG("%s\n", "ok");
		
		open_file = open_file->next;
	}
	
	/* relock files */
	if (resume_failed == 0)
	{
		const struct list *lock_item = instance->resume.locked_files;
		while (lock_item != NULL)
		{
			const struct lock_rec *lock_info = (const struct lock_rec *)(lock_item->data);

			if (lock_info->fully_locked == 0) /* we're not supporting relocking of files 
			which are not locked for the full length */
			{
				ret = -EBADF;
				resume_failed = 1; 

				break;
			}
				
			DEBUG("relocking file %s\n", lock_info->path);

			struct flock fl = { 0 };
			fl.l_type = lock_info->lock_type;
			fl.l_whence = SEEK_SET;
			fl.l_start = 0;
			fl.l_len = 0;

			uint64_t desc = resume_is_file_in_open_list(instance->resume.open_files, lock_info->path);
			if (desc == (uint64_t)-1) /* we can only resume files which were opened 
			in other case, we don't know which open flags were used to lock that file and etc 
			so we can't reopen file on our own */
			{
				ret = -EBADF;
				resume_failed = 1;

				break;
			}

			int lock_ret = _rfs_lock(instance, lock_info->path, desc, F_SETLK, &fl);
				
			if (lock_ret < 0)
			{
				ret = lock_ret;
				resume_remove_file_from_locked_list(&instance->resume.locked_files, lock_info->path);

				resume_failed = 1;
				break;
			}
		
			DEBUG("%s\n", "ok");

			lock_item = lock_item->next;
		}
	}
		
	/* if resume failed, then close all files marked as open 
	and unlock locked files*/
	if (resume_failed != 0)
	{
		DEBUG("%s\n", "resume failed");

		const struct list *locked_file = instance->resume.locked_files;
		while (locked_file != NULL)
		{
			struct lock_rec *data = (struct lock_rec *)locked_file->data;
			
			struct flock fl = { 0 };
			fl.l_type = data->lock_type;
			fl.l_whence = SEEK_SET;
			fl.l_start = 0;
			fl.l_len = 0;

			uint64_t desc = resume_is_file_in_open_list(instance->resume.open_files, data->path);

			if (desc != (uint64_t)-1)
			{
				_rfs_lock(instance, data->path, desc, F_UNLCK, &fl); /* ignore the result and keep going */
			}
				
			resume_remove_file_from_locked_list(&instance->resume.locked_files, data->path);
			
			locked_file = locked_file->next;
		}

		const struct list *open_file = instance->resume.open_files;
		while (open_file != NULL)
		{
			struct open_rec *data = (struct open_rec *)open_file->data;
			
			_rfs_release(instance, data->path, data->desc); /* ignore the result and keep going */

			resume_remove_file_from_open_list(&instance->resume.open_files, data->path);
			
			open_file = open_file->next;
		}
	}
	
	return ret; /* not real error. probably. but we've tried our best */
}

