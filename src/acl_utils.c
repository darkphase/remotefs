/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifdef WITH_ACL

#include <sys/xattr.h>
#include <sys/acl.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "buffer.h"
#include "list.h"
#include "acl_utils.h"
#include "id_lookup.h"
#include "instance.h"
#include "acl/libacl/byteorder.h"

static inline void write_text(char **cursor, const char *text, size_t len)
{
	memcpy(*cursor, text, len);
	*cursor += len;
}

char* rfs_acl_to_xattr(const rfs_acl_t* acl, int count)
{
	size_t size = acl_ea_size(count);
	rfs_acl_t *value = get_buffer(size);
	
	value->a_version = cpu_to_le32(acl->a_version);
	int i = 0; for (i = 0; i < count; ++i)
	{
	    value->a_entries[i].e_tag = cpu_to_le16(acl->a_entries[i].e_tag);
	    value->a_entries[i].e_perm = cpu_to_le16(acl->a_entries[i].e_perm);
	    value->a_entries[i].e_id = cpu_to_le32(acl->a_entries[i].e_id);
	}
	
	return (char *)value;
}

rfs_acl_t* rfs_acl_from_xattr(const char *value, size_t size)
{
	if (size < sizeof(rfs_acl_t))
	{
		return NULL;
	}
	
	int count = acl_ea_count(size);
	
	DEBUG("count: %d\n", count);
	
	if (count < 1)
	{
		return NULL;
	}
	
	unsigned overall_size = sizeof(rfs_acl_t) + sizeof(rfs_acl_entry_t) * count;
	
	rfs_acl_t *acl = get_buffer(overall_size);
	if (acl == NULL)
	{
		return NULL;
	}
	
	memset(acl, 0, overall_size);
	
	rfs_acl_t *value_acl = (rfs_acl_t *)value;
	acl->a_version = le32_to_cpu(value_acl->a_version);
	
	int i = 0; for (i = 0; i < count; ++i)
	{
		acl->a_entries[i].e_tag = le16_to_cpu(value_acl->a_entries[i].e_tag);
		acl->a_entries[i].e_perm = le16_to_cpu(value_acl->a_entries[i].e_perm);
		
		switch(acl->a_entries[i].e_tag) 
		{
		case ACL_USER_OBJ:
		case ACL_GROUP_OBJ:
		case ACL_MASK:
		case ACL_OTHER:
			acl->a_entries[i].e_id = ACL_UNDEFINED_ID;
			break;
		case ACL_USER:
		case ACL_GROUP:
		        acl->a_entries[i].e_id = le32_to_cpu(value_acl->a_entries[i].e_id);
		        break;
		default:
			free_buffer(acl);
			return NULL;
		}
	}
	
	return acl;
}

char* rfs_acl_to_text(struct id_lookup_info *lookup, 
	const rfs_acl_t *acl, 
	int count, 
	size_t *len)
{
	if (len != NULL)
	{
		*len = 0;
	}

	/* at first pass just calc resulting size */
	size_t total_len = 0;
	unsigned i = 0; for (i = 0; i < count; ++i)
	{
		uint16_t type = acl->a_entries[i].e_tag;
		uint32_t id = acl->a_entries[i].e_id;
		
		switch (type)
		{
		case ACL_USER_OBJ:
		case ACL_USER:
			total_len += strlen(STR_USER_TAG);
			if (type == ACL_USER)
			{
				const char *username = get_uid_name(lookup->uids, (uid_t)id);
				if (username == NULL)
				{
					return NULL;
				}
				
				total_len += strlen(username);
			}
			total_len += strlen(STR_ACL_DELIMITER);
			break;
		case ACL_GROUP_OBJ:
		case ACL_GROUP:
			total_len += strlen(STR_GROUP_TAG);
			if (type == ACL_GROUP)
			{
				const char *groupname = get_gid_name(lookup->gids, (gid_t)id);
				if (groupname == NULL)
				{
					return NULL;
				}
				
				total_len += strlen(groupname);
			}
			total_len += strlen(STR_ACL_DELIMITER);
			break;
		case ACL_MASK:
			total_len += strlen(STR_MASK_TAG);
			total_len += strlen(STR_ACL_DELIMITER);
			break;
		case ACL_OTHER:
			total_len += strlen(STR_OTHER_TAG);
			total_len += strlen(STR_ACL_DELIMITER);
			break;
		default:
			return NULL;
		}
		total_len += strlen(STR_ACL_RWX);
		total_len += strlen(STR_ACL_DELIMITER);
	}
	
	char *text_acl = get_buffer(total_len + 1);
#ifdef RFS_DEBUG
	memset(text_acl, 0, total_len + 1);
#endif
	char *cursor = text_acl;
	
	/* second pass - fill in allocated buffer */
	i = 0; for (i = 0; i < count; ++i)
	{
		uint16_t type = acl->a_entries[i].e_tag;
		uint16_t perm = acl->a_entries[i].e_perm;
		uint32_t id = acl->a_entries[i].e_id;
		
		switch (type)
		{
		case ACL_USER_OBJ:
		case ACL_USER:
			write_text(&cursor, STR_USER_TAG, strlen(STR_USER_TAG));
			if (type == ACL_USER)
			{
				const char *username = get_uid_name(lookup->uids, (uid_t)id);
				if (username == NULL)
				{
					return NULL;
				}
				
				write_text(&cursor, username, strlen(username));
			}
			
			write_text(&cursor, STR_ACL_DELIMITER, strlen(STR_ACL_DELIMITER));
			break;
		case ACL_GROUP_OBJ:
		case ACL_GROUP:
			write_text(&cursor, STR_GROUP_TAG, strlen(STR_GROUP_TAG));
			if (type == ACL_GROUP)
			{
				const char *groupname = get_gid_name(lookup->gids, (gid_t)id);
				if (groupname == NULL)
				{
					return NULL;
				}
				
				write_text(&cursor, groupname, strlen(groupname));
			}
			
			write_text(&cursor, STR_ACL_DELIMITER, strlen(STR_ACL_DELIMITER));
			break;
		case ACL_MASK:
			write_text(&cursor, STR_MASK_TAG, strlen(STR_MASK_TAG));
			write_text(&cursor, STR_ACL_DELIMITER, strlen(STR_ACL_DELIMITER));
			break;
		case ACL_OTHER:
			write_text(&cursor, STR_OTHER_TAG, strlen(STR_OTHER_TAG));
			write_text(&cursor, STR_ACL_DELIMITER, strlen(STR_ACL_DELIMITER));
			break;
		default:
			free_buffer(text_acl);
			return NULL;
		}
		
		*cursor = ((perm & ACL_READ) != 0 ? 'r' : '-');
		++cursor;
		*cursor = ((perm & ACL_WRITE) != 0 ? 'w' : '-');
		++cursor;
		*cursor = ((perm & ACL_EXECUTE) != 0 ? 'x' : '-');
		++cursor;
		write_text(&cursor, STR_ACL_DELIMITER, strlen(STR_ACL_DELIMITER));
	}
	
	*cursor = 0; /* final zero */
	
	if (len != NULL)
	{
		*len = total_len;
	}
	
	return text_acl;
}

rfs_acl_t* rfs_acl_from_text(struct id_lookup_info *lookup,
	const char *text, 
	int *count)
{
	if (count != NULL)
	{
		*count = 0;
	}
	
	if (text == NULL)
	{
		return NULL;
	}
	
	int ret_count = 0;
	
	const char *cursor = text;
	
	/* first pass - calc entries count (and validate input) */
	while (*cursor != 0)
	{
		if (memcmp(cursor, STR_USER_TAG, strlen(STR_USER_TAG)) == 0)
		{
			cursor += strlen(STR_USER_TAG);
			if (memcmp(cursor, STR_ACL_DELIMITER, strlen(STR_ACL_DELIMITER)) != 0)
			{
				cursor = strstr(cursor, STR_ACL_DELIMITER);
				if (cursor == NULL)
				{
					return NULL;
				}
			}
			cursor += strlen(STR_ACL_DELIMITER);
		}
		else if (memcmp(cursor, STR_GROUP_TAG, strlen(STR_GROUP_TAG)) == 0)
		{
			cursor += strlen(STR_GROUP_TAG);
			if (memcmp(cursor, STR_ACL_DELIMITER, strlen(STR_ACL_DELIMITER)) != 0)
			{
				cursor = strstr(cursor, STR_ACL_DELIMITER);
				if (cursor == NULL)
				{
					return NULL;
				}
			}
			cursor += strlen(STR_ACL_DELIMITER);
		}
		else if (memcmp(cursor, STR_MASK_TAG, strlen(STR_MASK_TAG)) == 0)
		{
			cursor += strlen(STR_MASK_TAG);
			if (memcmp(cursor, STR_ACL_DELIMITER, strlen(STR_ACL_DELIMITER)) != 0)
			{
				return NULL;
			}
			cursor += strlen(STR_ACL_DELIMITER);
		}
		else if (memcmp(cursor, STR_OTHER_TAG, strlen(STR_OTHER_TAG)) == 0)
		{
			cursor += strlen(STR_OTHER_TAG);
			if (memcmp(cursor, STR_ACL_DELIMITER, strlen(STR_ACL_DELIMITER)) != 0)
			{
				return NULL;
			}
			cursor += strlen(STR_ACL_DELIMITER);
		}
		else
		{
			return NULL;
		}
		
		/* next should be rwx and delimiter */
		if (*cursor != 'r' && *cursor != '-')
		{
			return NULL;
		}
		++cursor;
		if (*cursor != 'w' && *cursor != '-')
		{
			return NULL;
		}
		++cursor;
		if (*cursor != 'x' && *cursor != '-')
		{
			return NULL;
		}
		++cursor;
		
		if (memcmp(cursor, STR_ACL_DELIMITER, strlen(STR_ACL_DELIMITER)) != 0)
		{
			return NULL;
		}
		cursor += strlen(STR_ACL_DELIMITER);
		
		++ret_count;
	}
	
	rfs_acl_t *acl = get_buffer(acl_ea_size(ret_count));
	acl->a_version = ACL_EA_VERSION;

	cursor = text;
	/* second pass - fill allocated buffer */
	int i = 0; for (i = 0; i < ret_count; ++i)
	{
		acl->a_entries[i].e_id = ACL_UNDEFINED_ID;
		acl->a_entries[i].e_tag = ACL_UNDEFINED_TAG;
		
		if (memcmp(cursor, STR_USER_TAG, strlen(STR_USER_TAG)) == 0)
		{
			cursor += strlen(STR_USER_TAG);
			if (memcmp(cursor, STR_ACL_DELIMITER, strlen(STR_ACL_DELIMITER)) == 0)
			{
				acl->a_entries[i].e_tag = ACL_USER_OBJ;
			}
			else
			{
				const char *username_end = strstr(cursor, STR_ACL_DELIMITER);
				unsigned username_len = username_end - cursor;
				char *username = get_buffer(username_len + 1);
				memcpy(username, cursor, username_len);
				username[username_len] = 0;
				
				uid_t uid = get_uid(lookup->uids, username);
				
				free_buffer(username);
				
				if (uid == (uid_t)-1)
				{
					free_buffer(acl);
					return NULL;
				}
				
				acl->a_entries[i].e_tag = ACL_USER;
				acl->a_entries[i].e_id = (uint32_t)uid;
				
				cursor = username_end;
			}
			cursor += strlen(STR_ACL_DELIMITER);
		}
		else if (memcmp(cursor, STR_GROUP_TAG, strlen(STR_GROUP_TAG)) == 0)
		{
			cursor += strlen(STR_GROUP_TAG);
			if (memcmp(cursor, STR_ACL_DELIMITER, strlen(STR_ACL_DELIMITER)) == 0)
			{
				acl->a_entries[i].e_tag = ACL_GROUP_OBJ;
				acl->a_entries[i].e_id = (uint32_t)-1;
			}
			else
			{
				const char *groupname_end = strstr(cursor, STR_ACL_DELIMITER);
				unsigned groupname_len = groupname_end - cursor;
				char *groupname = get_buffer(groupname_len + 1);
				memcpy(groupname, cursor, groupname_len);
				groupname[groupname_len] = 0;
				
				gid_t gid = get_gid(lookup->gids, groupname);
				
				free_buffer(groupname);
				
				if (gid == (gid_t)-1)
				{
					free_buffer(acl);
					return NULL;
				}
				
				acl->a_entries[i].e_tag = ACL_GROUP;
				acl->a_entries[i].e_id = (uint32_t)gid;
				
				cursor = groupname_end;
			}
			cursor += strlen(STR_ACL_DELIMITER);
		}
		else if (memcmp(cursor, STR_MASK_TAG, strlen(STR_MASK_TAG)) == 0)
		{
			acl->a_entries[i].e_tag = ACL_MASK;
			
			cursor += strlen(STR_MASK_TAG);
			cursor += strlen(STR_ACL_DELIMITER);
		}
		else if (memcmp(cursor, STR_OTHER_TAG, strlen(STR_OTHER_TAG)) == 0)
		{
			acl->a_entries[i].e_tag = ACL_OTHER;
			
			cursor += strlen(STR_OTHER_TAG);
			cursor += strlen(STR_ACL_DELIMITER);
		}
		else
		{
			free_buffer(acl);
			return NULL;
		}
		
		/* next should be rwx and delimiter */
		
		acl->a_entries[i].e_perm = (uint16_t)0;
		
		if (*cursor == 'r')
		{
			acl->a_entries[i].e_perm |= ACL_READ;
		}
		++cursor;
		if (*cursor == 'w')
		{
			acl->a_entries[i].e_perm |= ACL_WRITE;
		}
		++cursor;
		if (*cursor == 'x')
		{
			acl->a_entries[i].e_perm |= ACL_EXECUTE;
		}
		++cursor;
		
		cursor += strlen(STR_ACL_DELIMITER);
	}

	if (count != NULL)
	{
		*count = ret_count;
	}
	
	return acl;
}

#ifdef RFS_DEBUG
void dump_acl_entry(struct id_lookup_info *lookup, const rfs_acl_entry_t *entry)
{
	size_t total_size = 0;
	switch (entry->e_tag)
	{
	case ACL_USER_OBJ:
	case ACL_USER:
		total_size += strlen(STR_USER_TAG);
		if (entry->e_id != (uint32_t)-1)
		{
			const char *username = get_uid_name(lookup->uids, (uid_t)entry->e_id);
			if (username == NULL)
			{
				username = "NULL";
			}
			total_size += strlen(username);
		}
		total_size += strlen(STR_ACL_DELIMITER);
		break;
	case ACL_GROUP_OBJ:
	case ACL_GROUP:
		total_size += strlen(STR_GROUP_TAG);
		if (entry->e_id != (uint32_t)-1)
		{
			const char *groupname = get_gid_name(lookup->gids, (gid_t)entry->e_id);
			if (groupname == NULL)
			{
				groupname = "NULL";
			}
			total_size += strlen(groupname);
		}
		total_size += strlen(STR_ACL_DELIMITER);
		break;
	case ACL_MASK:
		total_size += strlen(STR_MASK_TAG);
		total_size += strlen(STR_ACL_DELIMITER);
		break;
	case ACL_OTHER:
		total_size += strlen(STR_OTHER_TAG);
		total_size += strlen(STR_ACL_DELIMITER);
		break;
	}
	
	total_size += strlen(STR_ACL_RWX);
	total_size += strlen(STR_ACL_DELIMITER);
	
	char *buffer = get_buffer(total_size + 1);
	char *cursor = buffer;
	
	switch (entry->e_tag)
	{
	case ACL_USER_OBJ:
	case ACL_USER:
		write_text(&cursor, STR_USER_TAG, strlen(STR_USER_TAG));
		if (entry->e_id != (uint32_t)-1)
		{
			const char *username = get_uid_name(lookup->uids, (uid_t)entry->e_id);
			if (username == NULL)
			{
				username = "NULL";
			}
			write_text(&cursor, username, strlen(username));
		}
		write_text(&cursor, STR_ACL_DELIMITER, strlen(STR_ACL_DELIMITER));
		break;
	case ACL_GROUP_OBJ:
	case ACL_GROUP:
		write_text(&cursor, STR_GROUP_TAG, strlen(STR_GROUP_TAG));
		if (entry->e_id != (uint32_t)-1)
		{
			const char *groupname = get_gid_name(lookup->gids, (gid_t)entry->e_id);
			if (groupname == NULL)
			{
				groupname = "NULL";
			}
			write_text(&cursor, groupname, strlen(groupname));
		}
		write_text(&cursor, STR_ACL_DELIMITER, strlen(STR_ACL_DELIMITER));
		break;
	case ACL_MASK:
		write_text(&cursor, STR_MASK_TAG, strlen(STR_MASK_TAG));
		write_text(&cursor, STR_ACL_DELIMITER, strlen(STR_ACL_DELIMITER));
		break;
	case ACL_OTHER:
		write_text(&cursor, STR_OTHER_TAG, strlen(STR_OTHER_TAG));
		write_text(&cursor, STR_ACL_DELIMITER, strlen(STR_ACL_DELIMITER));
		break;
	}
	
	*cursor = ((entry->e_perm & ACL_READ) != 0 ? 'r' : '-');
	++cursor;
	
	*cursor = ((entry->e_perm & ACL_WRITE) != 0 ? 'w' : '-');
	++cursor;
	
	*cursor = ((entry->e_perm & ACL_EXECUTE) != 0 ? 'x' : '-');
	++cursor;
	
	write_text(&cursor, STR_ACL_DELIMITER, strlen(STR_ACL_DELIMITER));
	
	*cursor = 0;
	
	DEBUG("%s\n", buffer);
}

void dump_acl(struct id_lookup_info *lookup, const rfs_acl_t *acl, int count)
{
	DEBUG("%s\n", "dumping acl entry:");
	DEBUG("version: %d\n", acl->a_version);
	int i = 0; for (i = 0; i < count; ++i)
	{
		dump_acl_entry(lookup, &acl->a_entries[i]);
	}
}
#endif

#endif /* WITH_ACL */
