/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>

#include "buffer.h"
#include "config.h"
#include "id_lookup_client.h"
#include "instance_client.h"
#include "operations_utils.h"

uint16_t rfs_file_flags(int os_flags)
{
	uint16_t flags = 0;
	
	if (os_flags & O_APPEND)       { flags |= RFS_APPEND; }
	if (os_flags & O_ASYNC)        { flags |= RFS_ASYNC; }
	if (os_flags & O_CREAT)        { flags |= RFS_CREAT; }
	if (os_flags & O_EXCL)         { flags |= RFS_EXCL; }
	if (os_flags & O_NONBLOCK)     { flags |= RFS_NONBLOCK; }
	if (os_flags & O_NDELAY)       { flags |= RFS_NDELAY; }
	if (os_flags & O_SYNC)         { flags |= RFS_SYNC; }
	if (os_flags & O_TRUNC)        { flags |= RFS_TRUNC; }
#if defined DARWIN /* is ok for linux too */
	switch (os_flags & O_ACCMODE)
	{
		case O_RDONLY: flags |= RFS_RDONLY; break;
		case O_WRONLY: flags |= RFS_WRONLY; break;
		case O_RDWR:   flags |= RFS_RDWR;   break;
	}
#else
	if (os_flags & O_RDONLY)       { flags |= RFS_RDONLY; }
	if (os_flags & O_WRONLY)       { flags |= RFS_WRONLY; }
	if (os_flags & O_RDWR)         { flags |= RFS_RDWR; }
#endif

	return flags;
}

size_t stat_size()
{
	return 0
	+ sizeof(uint32_t) /* mode */
	+ sizeof(uint32_t) /* user_len */
	+ sizeof(uint32_t) /* group_len */
	+ sizeof(uint64_t) /* size */
	+ sizeof(uint64_t) /* atime */
	+ sizeof(uint64_t) /* mtime */
	+ sizeof(uint64_t) /* ctime */
	+ sizeof(uint32_t) /* nlink */
	+ sizeof(uint32_t) /* blocks */
	;
}

off_t unpack_stat(struct rfs_instance *instance, const char *buffer, struct stat *result, int *ret)
{
	uint32_t mode = 0;
	uint32_t user_len = 0;
	uint32_t group_len = 0;
	uint64_t size = 0;
	uint64_t atime = 0;
	uint64_t mtime = 0;
	uint64_t ctime = 0;
	uint32_t nlink = 0;
	uint32_t blocks = 0;
	const char *user = NULL;
	const char *group = NULL;

	off_t last_pos = 
	unpack_32(&blocks, buffer, 
	unpack_32(&nlink, buffer, 
	unpack_64(&ctime, buffer, 
	unpack_64(&mtime, buffer, 
	unpack_64(&atime, buffer, 
	unpack_64(&size, buffer, 
	unpack_32(&group_len, buffer, 
	unpack_32(&user_len, buffer, 
	unpack_32(&mode, buffer, 0 
	)))))))));
	
	user = buffer + last_pos;
	group = buffer + last_pos + user_len;
	
	if (strlen(user) + 1 != user_len
	|| strlen(group) + 1 != group_len)
	{
		*ret = EBADMSG;
		return (off_t)-1;
	}

	last_pos += user_len + group_len;
	
#ifdef WITH_UGO
	uid_t uid = (uid_t)-1;
	gid_t gid = (gid_t)-1;
	
	if ((instance->client.export_opts & OPT_UGO) != 0)
	{
		size_t host_len = strlen(instance->config.host);

		/* if this is file owned by auth_user, then set uid to my_uid, 
		otherwise: */
		if (strcmp(user, instance->config.auth_user) != 0)
		{
			/* rfsnss support: try to find user with @host before looking up exact user reported by server */
			size_t user_len = strlen(user);
			size_t overall_user_len = user_len + host_len + 1 + 1; /* + '@' + final \0 */

			char *remote_user = get_buffer(overall_user_len);
			snprintf(remote_user, overall_user_len, "%s@%s", user, instance->config.host);

			DEBUG("remote user: %s\n", remote_user);
		
			struct passwd *pw = getpwnam(remote_user);
		
			free_buffer(remote_user);
		
			if (pw != NULL)
			{
				uid = pw->pw_uid;
			}
		}
		else
		{
			uid = instance->client.my_uid;
		}

		/* rfsnss support: see comments for username handling */
		size_t group_len = strlen(group);
		size_t overall_group_len = group_len + host_len + 1 + 1;

		char *remote_group = get_buffer(overall_group_len);
		snprintf(remote_group, overall_group_len, "%s@%s", group, instance->config.host);

		DEBUG("remote group: %s\n", remote_group);

		struct group *gr = getgrnam(remote_group);

		free_buffer(remote_group);

		if (gr != NULL)
		{
			gid = gr->gr_gid;
		}

		DEBUG("intermediate uid: %d, gid: %d\n", uid, gid);

		if (uid == (uid_t)-1)
		{
			uid = lookup_user(user);
		}

		if (gid == (gid_t)-1)
		{
			gid = lookup_group(group, user);
		}
	}

	DEBUG("user: %s, group: %s, uid: %d, gid: %d\n", user, group, uid, gid);

	if ((instance->client.export_opts & OPT_UGO) != 0)
	{
		result->st_uid = uid;
		result->st_gid = gid;
	}
	else
#endif /* WITH_UGO */
	{
		result->st_uid = instance->client.my_uid;
		result->st_gid = instance->client.my_gid;
	}
	
	result->st_mode = mode;
	result->st_size = (off_t)size;
	result->st_atime = (time_t)atime;
	result->st_mtime = (time_t)mtime;
	result->st_ctime = (time_t)ctime;
	result->st_nlink = (nlink_t)nlink;
	result->st_blocks = (blkcnt_t)blocks;

	return last_pos;
}

