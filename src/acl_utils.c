/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifdef WITH_ACL

#include <errno.h>
#include <sys/acl.h>
#include <sys/xattr.h>
#include <string.h>

#include "acl_utils.h"
#include "buffer.h"
#include "config.h"
#include "id_lookup.h"
#include "instance.h"
#include "list.h"
#include "acl/libacl/byteorder.h"

static inline size_t write_text(char **cursor, const char *text, size_t len)
{
	if (cursor != NULL && *cursor != NULL)
	{
		memcpy(*cursor, text, len);
		*cursor += len;
	}

	return len;
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

int walk_acl(const rfs_acl_t *acl, size_t count, walk_acl_callback callback, void *data)
{
	DEBUG("walking ACL, count: %lu\n", (unsigned long)count);

	size_t i = 0; for (i = 0; i < count; ++i)
	{
		uint16_t type = acl->a_entries[i].e_tag;
		uint16_t perm = acl->a_entries[i].e_perm;
		uint32_t id = acl->a_entries[i].e_id;

		int callback_ret = callback(type, perm, id, data);
		if (callback_ret != 0)
		{
			return callback_ret;
		}
	}

	return 0;
}

int walk_acl_text(const char *acl_text, walk_acl_text_callback callback, void *data)
{
	const char *cursor = acl_text;
	const char *name = NULL;
	size_t name_len = 0;
	uint16_t type = ACL_UNDEFINED_TAG;
	uint16_t perm = 0;

	DEBUG("%s\n", "walking ACL text");

	while (*cursor != 0)
	{
		DEBUG("%s\n", cursor);

		name = NULL;
		name_len = 0;
		type = ACL_UNDEFINED_TAG;
		perm = 0;

		if (memcmp(cursor, STR_USER_TAG, strlen(STR_USER_TAG)) == 0)
		{
			cursor += strlen(STR_USER_TAG);
			if (memcmp(cursor, STR_ACL_DELIMITER, strlen(STR_ACL_DELIMITER)) != 0)
			{
				name = cursor;
				
				cursor = strstr(cursor, STR_ACL_DELIMITER);
				if (cursor == NULL)
				{
					return -EINVAL;
				}

				name_len = cursor - name; 
			}
			cursor += strlen(STR_ACL_DELIMITER);
			type = (name_len == 0 ? ACL_USER_OBJ : ACL_USER);
		}
		else if (memcmp(cursor, STR_GROUP_TAG, strlen(STR_GROUP_TAG)) == 0)
		{
			cursor += strlen(STR_GROUP_TAG);
			if (memcmp(cursor, STR_ACL_DELIMITER, strlen(STR_ACL_DELIMITER)) != 0)
			{
				name = cursor;

				cursor = strstr(cursor, STR_ACL_DELIMITER);
				if (cursor == NULL)
				{
					return -EINVAL;
				}

				name_len = cursor - name;
			}
			cursor += strlen(STR_ACL_DELIMITER);
			type = (name_len == 0 ? ACL_GROUP_OBJ : ACL_GROUP);
		}
		else if (memcmp(cursor, STR_MASK_TAG, strlen(STR_MASK_TAG)) == 0)
		{
			cursor += strlen(STR_MASK_TAG);
			if (memcmp(cursor, STR_ACL_DELIMITER, strlen(STR_ACL_DELIMITER)) != 0)
			{
				return -EINVAL;
			}
			cursor += strlen(STR_ACL_DELIMITER);
			type = ACL_MASK;
		}
		else if (memcmp(cursor, STR_OTHER_TAG, strlen(STR_OTHER_TAG)) == 0)
		{
			cursor += strlen(STR_OTHER_TAG);
			if (memcmp(cursor, STR_ACL_DELIMITER, strlen(STR_ACL_DELIMITER)) != 0)
			{
				return -EINVAL;
			}
			cursor += strlen(STR_ACL_DELIMITER);
			type = ACL_OTHER;
		}
		else
		{
			return -EINVAL;
		}
			
		uint16_t perm = 0;
		
		/* next should be rwx and delimiter */
		if (*cursor != 'r' && *cursor != '-')
		{
			return -EINVAL;
		}
		
		if (*cursor == 'r')
		{
			perm |= ACL_READ;
		}
		++cursor;

		if (*cursor != 'w' && *cursor != '-')
		{
			return -EINVAL;
		}
		
		if (*cursor == 'w')
		{
			perm |= ACL_WRITE;
		}
		++cursor;

		if (*cursor != 'x' && *cursor != '-')
		{
			return -EINVAL;
		}

		if (*cursor == 'x')
		{
			perm |= ACL_EXECUTE;
		}
		++cursor;
		
		if (*cursor != 0)
		{
			cursor += strlen(STR_ACL_DELIMITER);
		}
			
		int callback_ret = callback(type, perm, name, name_len, data);
		if (callback_ret != 0)
		{
			return callback_ret;
		}
	}

	return 0;
}

struct acl_to_text_params
{
	const struct id_lookup_info *lookup;
	size_t len;
	char *text;
	reverse_resolve custom_resolve;
	void *custom_resolve_data;
};

static char* default_reverse_resolve(uint16_t type, uint32_t id, void *lookup_casted)
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

static int acl_to_text_callback(uint16_t type, uint16_t perm, uint32_t id, void *params_casted)
{
	DEBUG("acl_to_text: type: %d, perm: %d, id: %d\n", type, perm, id);

	struct acl_to_text_params *params = (struct acl_to_text_params *)(params_casted);
	char *cursor = params->text;
	reverse_resolve resolve_func = (params->custom_resolve == NULL ? default_reverse_resolve : params->custom_resolve);
	void *resolve_data = (params->custom_resolve == NULL ? (void *)params->lookup : params->custom_resolve_data);

	switch (type)
	{
	case ACL_USER_OBJ:
	case ACL_USER:
		params->len += write_text(&cursor, STR_USER_TAG, strlen(STR_USER_TAG));

		if (type == ACL_USER)
		{	
			char *username = resolve_func(type, id, resolve_data);
			if (username == NULL)
			{
				return -EINVAL;
			}

			params->len += write_text(&cursor, username, strlen(username));
			free(username);
		}
		break;
	case ACL_GROUP_OBJ:
	case ACL_GROUP:
		params->len += write_text(&cursor, STR_GROUP_TAG, strlen(STR_GROUP_TAG));
		if (type == ACL_GROUP)
		{
			char *groupname = resolve_func(type, id, resolve_data);
			if (groupname == NULL)
			{
				return -EINVAL;
			}
			
			params->len += write_text(&cursor, groupname, strlen(groupname));
			free(groupname);
		}
		break;
	case ACL_MASK:
		params->len += write_text(&cursor, STR_MASK_TAG, strlen(STR_MASK_TAG));
		break;
	case ACL_OTHER:
		params->len += write_text(&cursor, STR_OTHER_TAG, strlen(STR_OTHER_TAG));
		break;
	default:
		return -EINVAL;
	}
		
	params->len += write_text(&cursor, STR_ACL_DELIMITER, strlen(STR_ACL_DELIMITER));
	
	if (cursor != NULL)
	{
		*cursor = ((perm & ACL_READ)    != 0 ? 'r' : '-'); ++cursor;
		*cursor = ((perm & ACL_WRITE)   != 0 ? 'w' : '-'); ++cursor;
		*cursor = ((perm & ACL_EXECUTE) != 0 ? 'x' : '-'); ++cursor;
		params->len += 3;
	}
	else
	{
		params->len += strlen(STR_ACL_RWX);
	}

	params->len += write_text(&cursor, STR_ACL_DELIMITER, strlen(STR_ACL_DELIMITER));

	if (cursor != NULL)
	{
		params->text = cursor;
	}
	
	return 0;
}

char* rfs_acl_to_text(const struct id_lookup_info *lookup, 
	const rfs_acl_t *acl, 
	size_t count, 
	reverse_resolve custom_resolve, 
	void *custom_resolve_data, 
	size_t *len)
{
	if (len != NULL)
	{
		*len = 0;
	}

	/* at first pass just calc resulting size */
	struct acl_to_text_params len_params = { lookup, 0, NULL, custom_resolve, custom_resolve_data };
	int len_ret = walk_acl(acl, count, acl_to_text_callback, (void *)&len_params);
	if (len_ret != 0)
	{
		return NULL;
	}

	DEBUG("calcuated ACL text len: %lu\n", (unsigned long)len_params.len);

	/* allocate memory for acl text */
	char *text_acl = get_buffer(len_params.len + 1);

	/* write to allocated entry */
	struct acl_to_text_params write_params = { lookup, 0, text_acl, custom_resolve, custom_resolve_data };
	int write_ret = walk_acl(acl, count, acl_to_text_callback, (void *)&write_params);
	
	DEBUG("written ACL text len: %lu\n", (unsigned long)write_params.len);

	if (write_ret != 0
	|| write_params.len != len_params.len)
	{
		free_buffer(text_acl);
		return NULL;
	}

	text_acl[write_params.len] = 0; /* final zero */
	
	if (len != NULL)
	{
		*len = write_params.len;
	}
	
	return text_acl;
}

struct acl_from_text_params
{
	const struct id_lookup_info *lookup;
	size_t count;
	rfs_acl_t *acl;
	resolve custom_resolve;
	void *custom_resolve_data;
};

static uint32_t default_resolve(uint16_t type, const char *name, size_t name_len, void *lookup_casted)
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

static int acl_from_text_callback(uint16_t type, uint16_t perm, const char *name, size_t name_len, void *params_casted)
{
	DEBUG("acl_from_text: type: %d, perm: %d, name: %s, name_len: %lu\n", type, perm, name, (long unsigned)name_len);

	struct acl_from_text_params *params = (struct acl_from_text_params *)(params_casted);

	if (params->acl != NULL)
	{
		resolve resolve_func = (params->custom_resolve == NULL ? default_resolve : params->custom_resolve);
		void *resolve_data = (params->custom_resolve == NULL ? (void *)params->lookup : params->custom_resolve_data);

		uint32_t id = ((name == NULL || name_len == 0) ? ACL_UNDEFINED_ID : resolve_func(type, name, name_len, resolve_data));

		switch (type)
		{
		case ACL_USER:
		case ACL_GROUP:
			if (id == ACL_UNDEFINED_ID)
			{
				return -ENOENT;
			}
			break;
		}

		params->acl->a_entries[params->count].e_id = id;
		params->acl->a_entries[params->count].e_tag = type;
		params->acl->a_entries[params->count].e_perm = perm;
	}

	++params->count;

	return 0;
}

rfs_acl_t* rfs_acl_from_text(const struct id_lookup_info *lookup,
	const char *text, 
	resolve custom_resolve, 
	void *custom_resolve_data, 
	size_t *count)
{
	if (count != NULL)
	{
		*count = 0;
	}
	
	if (text == NULL)
	{
		return NULL;
	}

	struct acl_from_text_params count_params = { lookup, 0, NULL, custom_resolve, custom_resolve_data };
	int count_ret = walk_acl_text(text, acl_from_text_callback, (void *)&count_params);
	if (count_ret != 0)
	{
		return NULL;
	}
	
	/* allocated counted number of ACL entries */
	rfs_acl_t *acl = get_buffer(acl_ea_size(count_params.count));
	acl->a_version = ACL_EA_VERSION;

	struct acl_from_text_params write_params = { lookup, 0, acl, custom_resolve, custom_resolve_data };
	int write_ret = walk_acl_text(text, acl_from_text_callback, (void *)&write_params);
	if (write_ret != 0)
	{
		free_buffer(acl);
		return NULL;
	}

	if (count != NULL)
	{
		*count = write_params.count;
	}
	
	return acl;
}

#ifdef RFS_DEBUG
void dump_acl(const struct id_lookup_info *lookup, const rfs_acl_t *acl, int count)
{
	DEBUG("%s\n", "dumping acl entry:");
	DEBUG("count: %d\n", count);
	DEBUG("version: %d\n", acl->a_version);

	char *acl_text = rfs_acl_to_text(lookup, acl, count, NULL, NULL, NULL);

	if (acl_text != NULL)
	{
		DEBUG("text: %s\n", acl_text);
		free(acl_text);
	}
}
#endif
#else
int acl_utils_dummy = 0;

#endif /* WITH_ACL */
