/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include "options.h"

#ifdef ACL_AVAILABLE

#include <errno.h>
#include <string.h>

#include "acl_utils.h"
#include "buffer.h"
#include "config.h"
#include "id_lookup.h"
#include "instance.h"

char* id_lookup_reverse_resolve(uint16_t type, uint32_t id, void *lookup_casted)
{
	struct id_lookup_info *lookup = (struct id_lookup_info *)(lookup_casted);

	switch (type)
	{
	case ACL_USER:
		{
		DEBUG("resolving username with id: %lu\n", (unsigned long)id);
		
		const char *username = get_uid_name(lookup->uids, (uid_t)id);
				
		if (username == NULL)
		{
			return NULL;
		}

		return strdup(username);
		}
	case ACL_GROUP:
		{
		DEBUG("resolving groupname with id: %lu\n", (unsigned long)id);

		const char *groupname = get_gid_name(lookup->gids, (gid_t)id);
				
		if (groupname == NULL)
		{
			return NULL;
		}
		
		return strdup(groupname);
		}
	}

	return NULL;
}

uint32_t id_lookup_resolve(uint16_t type, const char *name, size_t name_len, void *lookup_casted)
{
	struct id_lookup_info *lookup = (struct id_lookup_info *)(lookup_casted);

	switch (type)
	{
	case ACL_USER:
		{
		char *username = get_buffer(name_len + 1);
		memcpy(username, name, name_len);
		username[name_len] = 0;
	
		DEBUG("resolving username: %s\n", username);
		
		uid_t uid = get_uid(lookup->uids, username);
				
		free_buffer(username);
				
		if (uid == (uid_t)-1)
		{
			return ACL_UNDEFINED_ID;
		}

		return (uint32_t)(uid);
		}
	case ACL_GROUP:
		{
		char *groupname = get_buffer(name_len + 1);
		memcpy(groupname, name, name_len);
		groupname[name_len] = 0;
				
		DEBUG("resolving groupname: %s\n", groupname);

		gid_t gid = get_gid(lookup->gids, groupname);
				
		free_buffer(groupname);
				
		if (gid == (gid_t)-1)
		{
			return ACL_UNDEFINED_ID;
		}
		
		return (uint32_t)(gid);
		}
	}

	return ACL_UNDEFINED_ID;
}

#endif /* ACL_AVAILABLE */

