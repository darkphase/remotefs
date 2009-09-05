#include "options.h"

#ifdef ACL_AVAILABLE

#include <sys/xattr.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "acl_utils.h"
#include "acl_utils_nss.h"
#include "buffer.h"
#include "config.h"
#include "command.h"
#include "id_lookup_client.h"
#include "instance_client.h"
#include "operations_rfs.h"
#include "sendrecv_client.h"

static uint32_t local_resolve(uint16_t type, const char *name, size_t name_len, void *instance_casted)
{
	char *dup_name = buffer_dup_str(name, name_len);
	if (dup_name == NULL)
	{
		return ACL_UNDEFINED_ID;
	}

	DEBUG("locally resolving name: %s\n", dup_name);

	uint32_t id = ACL_UNDEFINED_ID;

	switch (type)
	{
	case ACL_USER:
		id = (uint32_t)(lookup_user(dup_name));
		break;
	case ACL_GROUP:
		id = (uint32_t)(lookup_group(dup_name, NULL));
		break;
	}

	free_buffer(dup_name);

	DEBUG("id: %lu\n", (unsigned long)id);
	return id;
}

int _rfs_getxattr(struct rfs_instance *instance, const char *path, const char *name, char *value, size_t size)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}
	
	if (strcmp(name, ACL_EA_ACCESS) != 0
	&& strcmp(name, ACL_EA_DEFAULT) != 0)
	{
		return -ENOTSUP;
	}
	
	uint32_t path_len = strlen(path) + 1;
	uint32_t name_len = strlen(name) + 1;
	uint64_t value_size = (uint64_t)size;
	
	unsigned overall_size = 
	+ sizeof(path_len) 
	+ sizeof(name_len) 
	+ sizeof(value_size) 
	+ path_len 
	+ name_len;
	
	char *buffer = get_buffer(overall_size);
	
	pack(name, name_len, 
	pack(path, path_len, 
	pack_64(&value_size, 
	pack_32(&name_len, 
	pack_32(&path_len, buffer
	)))));
	
	struct command cmd = { cmd_getxattr, overall_size };
	
	send_token_t token = { 0, {{ 0 }} };
	if (do_send(&instance->sendrecv, 
		queue_data(buffer, overall_size, 
		queue_cmd(&cmd, &token))) < 0)
	{
		free_buffer(buffer);
		return -ECONNABORTED;
	}
	
	free_buffer(buffer);
	
	struct answer ans = { 0 };
	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}
	
	if (ans.command != cmd_getxattr)
	{
		return cleanup_badmsg(instance, &ans);
	}
	
	if (ans.data_len > 0 && ans.ret >= 0)
	{
		buffer = get_buffer(ans.data_len);
		
		if (rfs_receive_data(&instance->sendrecv, buffer, ans.data_len) == -1)
		{
			free_buffer(buffer);
			return -ECONNABORTED;
		}
		
		DEBUG("acl: %s\n", buffer);

		size_t count = 0;
		rfs_acl_t *acl = rfs_acl_from_text(buffer, 
			(instance->nss.use_nss ? nss_resolve : local_resolve), 
			(void *)instance, 
			&count);

		free_buffer(buffer);
		
		if (acl == NULL)
		{
			return -EINVAL;
		}

		if (acl_ea_size(count) > size)
		{
			free_buffer(acl);
			return -ERANGE;
		}

		char *acl_value = rfs_acl_to_xattr(acl, count);
		if (acl_value == NULL)
		{
			free_buffer(acl);
			return -EINVAL;
		}
		
		memcpy(value, acl_value, acl_ea_size(count));
		
#ifdef RFS_DEBUG
		dump(value, acl_ea_size(count));
#endif
		
		ans.ret = acl_ea_size(count);
		
		free_buffer(acl);
		free_buffer(acl_value);
	}

	if (ans.ret_errno == ENOTSUP)
	{
		ans.ret = 0; /* fake ACL absense if server is reporting ENOTSUP: 
		it usualy means that "ugo" isn't enabled for mounted export */
	}

	return ans.ret >= 0 ? ans.ret : -ans.ret_errno;
}

static char* local_reverse_resolve(uint16_t type, uint32_t id, void *instance_casted)
{
	DEBUG("locally reverse resolving id: %lu\n", (unsigned long)id);

	char *name = NULL;
	const char *looked_up_name = NULL;

	switch (type)
	{
	case ACL_USER:
		looked_up_name = lookup_uid((uid_t)id);
		break;
	case ACL_GROUP:
		looked_up_name = lookup_gid((gid_t)id, (uid_t)(-1));
		break;
	}

	if (looked_up_name != NULL)
	{
		name = strdup(looked_up_name);
	}

	return name;
}

int _rfs_setxattr(struct rfs_instance *instance, const char *path, const char *name, const char *value, size_t size, int flags)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}
	
	if (strcmp(name, ACL_EA_ACCESS) != 0
	&& strcmp(name, ACL_EA_DEFAULT) != 0)
	{
		return -ENOTSUP;
	}

	errno = 0;
	rfs_acl_t *acl = rfs_acl_from_xattr(value, size);
	
	char *text_acl = NULL; 
	int count = acl_ea_count(size);
	size_t text_acl_len = 0;
	
	if (acl != NULL)
	{
		text_acl = rfs_acl_to_text(acl, 
			count, 
			(instance->nss.use_nss ? nss_reverse_resolve : local_reverse_resolve), 
			(void *)instance, 
			&text_acl_len);
	}
	else
	{
		return -EINVAL;
	}
	
	if (acl != NULL)
	{
		free_buffer(acl);
	}
	
	if (text_acl == NULL)
	{
		return -EINVAL;
	}
	
	DEBUG("acl: %s\n", text_acl);

	uint32_t path_len = strlen(path) + 1;
	uint32_t acl_flags = 0;
	if ((flags & XATTR_CREATE) != 0)
	{
		acl_flags |= RFS_XATTR_CREATE;
	}
	else if ((flags & XATTR_REPLACE) != 0)
	{
		acl_flags |= RFS_XATTR_REPLACE;
	}
	uint32_t name_len = strlen(name) + 1;
	
	unsigned overall_size = 
	+ sizeof(path_len) 
	+ sizeof(name_len) 
	+ sizeof(acl_flags) 
	+ path_len 
	+ name_len
	+ text_acl_len + 1;
	
	char *buffer = get_buffer(overall_size);
	
	pack(text_acl, text_acl_len + 1, 
	pack(name, name_len, 
	pack(path, path_len, 
	pack_32(&acl_flags, 
	pack_32(&name_len, 
	pack_32(&path_len, buffer
	))))));
	
	struct command cmd = { cmd_setxattr, overall_size };
	
	send_token_t token = { 0, {{ 0 }} };
	if (do_send(&instance->sendrecv, 
		queue_data(buffer, overall_size, 
		queue_cmd(&cmd, &token))) < 0)
	{
		free_buffer(text_acl);
		free_buffer(buffer);
		return -ECONNABORTED;
	}
	
	free_buffer(text_acl);
	free_buffer(buffer);
	
	struct answer ans = { 0 };
	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}
	
	if (ans.command != cmd_setxattr 
	|| ans.data_len != 0)
	{
		return cleanup_badmsg(instance, &ans);
	}
	
	return ans.ret == 0 ? 0 : -ans.ret_errno;
}
#else
int operations_acl_c_empty_module_makes_suncc_angry = 0; /* avoid warning about empty module */
#endif /* ACL_AVAILABLE */

