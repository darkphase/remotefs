#ifdef WITH_ACL

#include <sys/xattr.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "acl_utils.h"
#include "acl_utils_nss.h"
#include "buffer.h"
#include "config.h"
#include "command.h"
#include "instance_client.h"
#include "operations_rfs.h"
#include "sendrecv.h"

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
	
	pack(name, name_len, buffer, 
	pack(path, path_len, buffer, 
	pack_64(&value_size, buffer, 
	pack_32(&name_len, buffer, 
	pack_32(&path_len, buffer, 0
	)))));
	
	struct command cmd = { cmd_getxattr, overall_size };
	
	if (rfs_send_cmd_data(&instance->sendrecv, &cmd, buffer) == -1)
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
		rfs_acl_t *acl = rfs_acl_from_text(&instance->id_lookup, 
			buffer, 
			(instance->nss.use_nss ? nss_resolve : NULL), 
			(instance->nss.use_nss ? (void *)instance : NULL), 
			&count);

		free_buffer(buffer);
		
		if (acl == NULL)
		{
			return -EINVAL;
		}

#ifdef RFS_DEBUG
		dump_acl(&instance->id_lookup, acl, count);
#endif

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
	
	return ans.ret >= 0 ? ans.ret : -ans.ret_errno;
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
#ifdef RFS_DEBUG
		dump_acl(&instance->id_lookup, acl, count);
#endif
		text_acl = rfs_acl_to_text(&instance->id_lookup, 
			acl, 
			count, 
			(instance->nss.use_nss ? nss_reverse_resolve : NULL), 
			(instance->nss.use_nss ? (void *)instance : NULL), 
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
	
	pack(text_acl, text_acl_len + 1, buffer, 
	pack(name, name_len, buffer, 
	pack(path, path_len, buffer, 
	pack_32(&acl_flags, buffer, 
	pack_32(&name_len, buffer, 
	pack_32(&path_len, buffer, 0
	))))));
	
	struct command cmd = { cmd_setxattr, overall_size };
	
	if (rfs_send_cmd_data(&instance->sendrecv, &cmd, buffer) == -1)
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
#endif /* WITH_ACL */

