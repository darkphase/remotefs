#include "options.h"

#if defined ACL_AVAILABLE

#include <errno.h>
#include <string.h>
#include <sys/xattr.h>

#include "acl_utils.h"
#include "acl_utils_server.h"
#include "buffer.h"
#include "command.h"
#include "config.h"
#include "instance_server.h"
#include "sendrecv_server.h"
#include "server.h"

int _handle_getxattr(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);

	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(&instance->sendrecv, buffer, cmd->data_len) == -1)
	{
		free_buffer(buffer);
		return -1;
	}

	uint32_t path_len = 0;
	uint32_t name_len = 0;
	uint64_t value_size = 0;

	const char *path = 
	unpack_64(&value_size, 
	unpack_32(&name_len, 
	unpack_32(&path_len, buffer
	)));
	
	DEBUG("value size: %lld\n", (long long)value_size);
	
	if (strlen(path) + 1 != path_len)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EBADMSG) == 0 ? 1 : -1;
	}
	
	DEBUG("path: %s\n", path);
	
	const char *name = path + path_len;
	
	if (strlen(name) + 1 != name_len)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EBADMSG) == 0 ? 1 : -1;
	}
	
	DEBUG("name: %s\n", name);
	
	if (strcmp(name, ACL_EA_ACCESS) != 0
	&& strcmp(name, ACL_EA_DEFAULT) != 0)
	{
		/* not supported now */
		free_buffer(buffer);
		return reject_request(instance, cmd, ENOTSUP) == 0 ? 1 : -1;
	}
	
	char *value_buffer = NULL;
	if (value_size > 0)
	{
		value_buffer = get_buffer((size_t)value_size);
		if (value_buffer == NULL)
		{
			free_buffer(buffer);
			return reject_request(instance, cmd, ECANCELED) == 0 ? 1 : -1;
		}
	}
	
	errno = 0;
	int ret = getxattr(path, name, value_buffer, (size_t)value_size);
	int saved_errno = errno;
	
	free_buffer(buffer);
	if (ret <= 0)
	{
		if (value_buffer != NULL)
		{
			free_buffer(value_buffer);
		}
		return reject_request(instance, cmd, saved_errno) == 0 ? 1 : -1;
	}
	
	char *text_acl = NULL;
	size_t text_acl_len = 0;
	
	if (value_buffer != NULL && ret > 0)
	{
		rfs_acl_t *acl = rfs_acl_from_xattr(value_buffer, (size_t)ret);
		
		free_buffer(value_buffer);
		
		if (acl == NULL)
		{
			return reject_request(instance, cmd, ECANCELED) == 0 ? 1 : -1;
		}
		
		int count = acl_ea_count((size_t)ret);
		text_acl = rfs_acl_to_text(acl, 
			count,
			id_lookup_reverse_resolve, 
			(void *)&instance->id_lookup, 
			&text_acl_len);
		
		if (text_acl == NULL)
		{
			free_buffer(acl);
			return reject_request(instance, cmd, ECANCELED) == 0 ? 1 : -1;
		}
		
		DEBUG("acl: %s\n", text_acl);
		
		free_buffer(acl);
	}
	
	uint32_t ans_ret = (text_acl_len > 0 ? 0 : ret);
	uint32_t ans_data_len =  text_acl_len > 0 ? text_acl_len + 1 : 0;

	struct answer ans = { cmd_getxattr, ans_data_len, ans_ret, saved_errno };

	if (ans.data_len == 0)
	{
		if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
		{
			free_buffer(text_acl);
			return -1;
		}
	}
	else
	{
		send_token_t token = { 0, {{ 0 }} };
		if (do_send(&instance->sendrecv, 
			queue_data(text_acl, ans_data_len, 
			queue_ans(&ans, &token))) < 0)
		{
			free_buffer(text_acl);
			return -1;
		}
	}
	
	free_buffer(text_acl);
	
	return 0;
}

int _handle_setxattr(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = get_buffer(cmd->data_len);
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(&instance->sendrecv, buffer, cmd->data_len) == -1)
	{
		free_buffer(buffer);
		return -1;
	}
	
	uint32_t path_len = 0;
	uint32_t name_len = 0;
	uint32_t acl_flags = 0;

	const char *path = 
	unpack_32(&acl_flags, 
	unpack_32(&name_len, 
	unpack_32(&path_len, buffer
	)));
	
	DEBUG("flags: %u\n", acl_flags);

	if (strlen(path) + 1 != path_len)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EBADMSG) == 0 ? 1 : -1;
	}
	
	DEBUG("path: %s\n", path);
	
	const char *name = path + path_len;
	
	if (strlen(name) + 1 != name_len)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, EBADMSG) == 0 ? 1 : -1;
	}
	
	DEBUG("name: %s\n", name);
	
	if (strcmp(name, ACL_EA_ACCESS) != 0
	&& strcmp(name, ACL_EA_DEFAULT) != 0)
	{
		/* not supported now */
		free_buffer(buffer);
		return reject_request(instance, cmd, ENOTSUP) == 0 ? 1 : -1;
	}
	
	int flags = 0;
	
	if ((acl_flags & RFS_XATTR_CREATE) != 0)
	{
		flags |= XATTR_CREATE;
	}
	else if ((acl_flags & RFS_XATTR_REPLACE) != 0)
	{
		flags |= XATTR_REPLACE;
	}
	
	const char *text_acl = name + name_len;
	
	DEBUG("acl: %s\n", text_acl);
	
	size_t count = 0;
	rfs_acl_t *acl = rfs_acl_from_text(text_acl, 
		id_lookup_resolve, 
		(void *)&instance->id_lookup, 
		&count);
	
	if (acl == NULL)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, ECANCELED) == 0 ? 1 : -1;
	}
	
	char *value = rfs_acl_to_xattr(acl, count);
	size_t value_len = acl_ea_size(count);
	
	if (value == NULL)
	{
		free_buffer(acl);
		free_buffer(buffer);
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
	
	errno = 0;
	int ret = setxattr(path, name, value, value_len, flags);
	
	free_buffer(acl);
	free_buffer(value);
	free_buffer(buffer);
	
	struct answer ans = { cmd_setxattr, 0, ret, errno };
	if (rfs_send_answer(&instance->sendrecv, &ans) == -1)
	{
		return -1;
	}

	return 0;
}

#else
int server_handlers_acl_c_empty_module_makes_suncc_angry = 0; /* avoid warning about empty module */
#endif /* ACL_AVAILABLE */

