/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include "../options.h"

#ifdef ACL_AVAILABLE

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#ifdef LINUX
#	include <acl/libacl.h>
#endif

#include "utils.h"
#include "../buffer.h"
#include "../config.h"
#include "../id_lookup.h"
#include "../instance.h"

#if (defined FREEBSD || defined DARWIN)
#define acl_get_perm acl_get_perm_np
#endif

static inline size_t write_text(char **cursor, const char *text, size_t len)
{
	if (cursor != NULL && *cursor != NULL)
	{
		memcpy(*cursor, text, len);
		*cursor += len;
	}

	return len;
}

int walk_acl(const acl_t acl, walk_acl_callback callback, void *data)
{
	DEBUG("%s\n", "walking ACL");

	int i = ACL_FIRST_ENTRY;
	do
	{
		acl_entry_t acl_entry = NULL;

		errno = 0;
		if (acl_get_entry(acl, i, &acl_entry) != 1)
		{
			if (errno != 0)
			{
				return -errno;
			}

			break;
		}

		acl_tag_t tag = ACL_UNDEFINED_TAG;
		void *id = NULL;
		int perms = 0;

		errno = 0;
		if (acl_get_tag_type(acl_entry, &tag) != 0)
		{
			return -errno;
		}
		
		if (tag == ACL_USER 
		|| tag == ACL_GROUP)
		{
			errno = 0;
			id = acl_get_qualifier(acl_entry);
			if (id == NULL)
			{
				return -errno;
			}
		}

		acl_permset_t permset = NULL;

		errno = 0;
		if (acl_get_permset(acl_entry, &permset) != 0)
		{
			acl_free(id);
			return -errno;
		}

		if (acl_get_perm(permset, ACL_READ) == 1)    perms |= ACL_READ;
		if (acl_get_perm(permset, ACL_WRITE) == 1)   perms |= ACL_WRITE;
		if (acl_get_perm(permset, ACL_EXECUTE) == 1) perms |= ACL_EXECUTE;
		
		int callback_ret = callback(tag, perms, id, data);
		if (callback_ret != 0)
		{
			acl_free(id);
			DEBUG("acl_to_text callback returned %s\n", strerror(-callback_ret));
			return callback_ret;
		}

		acl_free(id);

		i = ACL_NEXT_ENTRY;
	}
	while (1);
	
	return 0;
}

int walk_acl_text(const char *acl_text, walk_acl_text_callback callback, void *data)
{
	const char *cursor = acl_text;

	DEBUG("%s\n", "walking ACL text");

	while (*cursor != 0)
	{
		DEBUG("%s\n", cursor);

		const char *name = NULL;
		size_t name_len = 0;
		acl_tag_t tag = ACL_UNDEFINED_TAG;
		int perms = 0;

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
			tag = (name_len == 0 ? ACL_USER_OBJ : ACL_USER);
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
			tag = (name_len == 0 ? ACL_GROUP_OBJ : ACL_GROUP);
		}
		else if (memcmp(cursor, STR_MASK_TAG, strlen(STR_MASK_TAG)) == 0)
		{
			cursor += strlen(STR_MASK_TAG);
			if (memcmp(cursor, STR_ACL_DELIMITER, strlen(STR_ACL_DELIMITER)) != 0)
			{
				return -EINVAL;
			}
			cursor += strlen(STR_ACL_DELIMITER);
			tag = ACL_MASK;
		}
		else if (memcmp(cursor, STR_OTHER_TAG, strlen(STR_OTHER_TAG)) == 0)
		{
			cursor += strlen(STR_OTHER_TAG);
			if (memcmp(cursor, STR_ACL_DELIMITER, strlen(STR_ACL_DELIMITER)) != 0)
			{
				return -EINVAL;
			}
			cursor += strlen(STR_ACL_DELIMITER);
			tag = ACL_OTHER;
		}
		else
		{
			return -EINVAL;
		}
			
		/* next should be rwx and delimiter */
		if (*cursor != 'r' && *cursor != '-')
		{
			return -EINVAL;
		}
		
		if (*cursor == 'r')
		{
			perms |= ACL_READ;
		}
		++cursor;

		if (*cursor != 'w' && *cursor != '-')
		{
			return -EINVAL;
		}
		
		if (*cursor == 'w')
		{
			perms |= ACL_WRITE;
		}
		++cursor;

		if (*cursor != 'x' && *cursor != '-')
		{
			return -EINVAL;
		}

		if (*cursor == 'x')
		{
			perms |= ACL_EXECUTE;
		}
		++cursor;
		
		if (*cursor != 0)
		{
			cursor += strlen(STR_ACL_DELIMITER);
		}
			
		int callback_ret = callback(tag, perms, name, name_len, data);
		if (callback_ret != 0)
		{
			DEBUG("acl_from_text callback returned %s\n", strerror(-callback_ret));
			return callback_ret;
		}
	}

	return 0;
}

struct acl_to_text_params
{
	char *text;
	size_t len;
	reverse_resolve custom_resolve;
	void *custom_resolve_data;
};

static int acl_to_text_callback(acl_tag_t tag, int perms, void *id, void *params_casted)
{
	DEBUG("acl_to_text: tag: %d, id: %d\n", (int)tag, id != NULL ? *(uint32_t *)id : ACL_UNDEFINED_ID);

	struct acl_to_text_params *params = (struct acl_to_text_params *)(params_casted);

	char *cursor = params->text;
	reverse_resolve resolve_func = params->custom_resolve;
	void *resolve_data = params->custom_resolve_data;

	switch (tag)
	{
	case ACL_USER_OBJ:
	case ACL_USER:
		params->len += write_text(&cursor, STR_USER_TAG, strlen(STR_USER_TAG));

		if (tag == ACL_USER)
		{	
			char *username = resolve_func(tag, id, resolve_data);
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
		if (tag == ACL_GROUP)
		{
			char *groupname = resolve_func(tag, id, resolve_data);
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
		*cursor = ((perms & ACL_READ)    != 0 ? 'r' : '-'); ++cursor;
		*cursor = ((perms & ACL_WRITE)   != 0 ? 'w' : '-'); ++cursor;
		*cursor = ((perms & ACL_EXECUTE) != 0 ? 'x' : '-'); ++cursor;
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

char* rfs_acl_to_text(const acl_t acl, 
	reverse_resolve custom_resolve, 
	void *custom_resolve_data, 
	size_t *len)
{
	if (len != NULL)
	{
		*len = 0;
	}

	/* at first pass just calc resulting size */
	struct acl_to_text_params len_params = { NULL, 0, custom_resolve, custom_resolve_data };
	int len_ret = walk_acl(acl, acl_to_text_callback, (void *)&len_params);
	if (len_ret != 0)
	{
		return NULL;
	}

	DEBUG("calcuated ACL text len: %lu\n", (unsigned long)len_params.len);

	/* allocate memory for acl text */
	char *text_acl = malloc(len_params.len + 1);

	/* write to allocated entry */
	struct acl_to_text_params write_params = { text_acl, 0, custom_resolve, custom_resolve_data };
	int write_ret = walk_acl(acl, acl_to_text_callback, (void *)&write_params);
	
	DEBUG("written ACL text len: %lu\n", (unsigned long)write_params.len);

	if (write_ret != 0
	|| write_params.len != len_params.len)
	{
		free(text_acl);
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
	acl_t *acl;
	size_t count;
	resolve custom_resolve;
	void *custom_resolve_data;
};

static int acl_from_text_callback(acl_tag_t tag, int perms, const char *name, size_t name_len, void *params_casted)
{
	DEBUG("acl_from_text: tag: %d, name: %s, name_len: %lu\n", tag, name, (long unsigned)name_len);

	struct acl_from_text_params *params = (struct acl_from_text_params *)(params_casted);

	if (params->acl != NULL)
	{
		resolve resolve_func = params->custom_resolve;
		void *resolve_data = params->custom_resolve_data;

		uint32_t id = ((name == NULL || name_len == 0) ? ACL_UNDEFINED_ID : resolve_func(tag, name, name_len, resolve_data));

		switch (tag)
		{
		case ACL_USER:
		case ACL_GROUP:
			if (id == ACL_UNDEFINED_ID)
			{
				return -ENOENT;
			}
			break;
		}

		acl_entry_t entry = NULL;
		errno = 0;
		if (acl_create_entry(params->acl, &entry) != 0)
		{
			DEBUG("%d\n", 1);
			return -errno;
		}
		
		errno = 0;
		if (acl_set_tag_type(entry, tag) != 0)
		{
			DEBUG("%d\n", 2);
			return -errno;
		}

		acl_permset_t permset = NULL;

		errno = 0;
		if (acl_get_permset(entry, &permset) != 0)
		{
			return -errno;
		}

		if ((perms & ACL_READ) != 0)
		{
			errno = 0;
			if (acl_add_perm(permset, ACL_READ) != 0)
			{
				return -errno;
			}
		}

		if ((perms & ACL_WRITE) != 0)
		{
			errno = 0;
			if (acl_add_perm(permset, ACL_WRITE) != 0)
			{
				return -errno;
			}
		}

		if ((perms & ACL_EXECUTE) != 0)
		{
			errno = 0;
			if (acl_add_perm(permset, ACL_EXECUTE) != 0)
			{
				return -errno;
			}
		}
		
		errno = 0;
		if (acl_set_permset(entry, permset) != 0)
		{
			return -errno;
		}

		switch (tag)
		{
		case ACL_USER:
			{
			uid_t uid = (uid_t)(id);
			errno = 0;
			if (acl_set_qualifier(entry, (void *)&uid) != 0)
			{
				DEBUG("%d\n", 4);
				return -errno;
			}
			}
		case ACL_GROUP:
			{
			gid_t gid = (gid_t)(id);
			errno = 0;
			if (acl_set_qualifier(entry, (void *)&gid) != 0)
			{
				DEBUG("%d\n", 5);
				return -errno;
			}
			}
			break;
		}
	}

	++params->count;

	return 0;
}

acl_t rfs_acl_from_text(const char *text, 
	resolve custom_resolve, 
	void *custom_resolve_data)
{
	
	if (text == NULL)
	{
		return NULL;
	}

	struct acl_from_text_params count_params = { NULL, 0, custom_resolve, custom_resolve_data };
	int count_ret = walk_acl_text(text, acl_from_text_callback, (void *)&count_params);
	if (count_ret != 0)
	{
		return NULL;
	}

	DEBUG("counted ACL entries: %lu\n", (unsigned long)count_params.count);
	
	/* allocate counted number of ACL entries */
	acl_t acl = acl_init(count_params.count);

	struct acl_from_text_params write_params = { &acl, 0, custom_resolve, custom_resolve_data };
	int write_ret = walk_acl_text(text, acl_from_text_callback, (void *)&write_params);
	if (write_ret != 0)
	{
		acl_free(acl);
		return NULL;
	}
	
	return acl;
}


#ifdef RFS_DEBUG
static int dump_acl_callback(acl_tag_t tag, int perms, void *id, void *params_casted)
{
	DEBUG("tag: %d, perms: %d, id: %lu\n", (int)(tag), perms, id != NULL ? *(unsigned long *)(id) : ACL_UNDEFINED_ID);
	return 0;
}

void dump_acl(const acl_t acl)
{
	DEBUG("acl valid: %d\n", acl_valid(acl) == 0 ? 1 : 0);
	walk_acl(acl, dump_acl_callback, NULL);
}
#endif

#else
int acl_utils_c_empty_module = 0;
#endif /* ACL_AVAILABLE */

