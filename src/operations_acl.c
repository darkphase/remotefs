#include "options.h"

#ifdef ACL_OPERATIONS_AVAILABLE

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/acl.h>

#include "acl/acl_linux.h"
#include "acl/acl_utils.h"
#include "acl/acl_utils_nss.h"
#include "buffer.h"
#include "config.h"
#include "command.h"
#include "id_lookup_client.h"
#include "instance_client.h"
#include "operations_rfs.h"
#include "sendrecv_client.h"

static uint32_t local_resolve(acl_tag_t tag, const char *name, size_t name_len, void *instance_casted)
{
	char *dup_name = buffer_dup_str(name, name_len);
	if (dup_name == NULL)
	{
		return ACL_UNDEFINED_ID;
	}

	DEBUG("locally resolving name: %s\n", dup_name);

	uint32_t id = ACL_UNDEFINED_ID;

	switch (tag)
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
	
	if (ans.ret_errno == ENOTSUP)
	{
		ans.ret = 0; /* fake ACL absense if server is reporting ENOTSUP: 
		it usualy means that "ugo" isn't enabled for mounted export */
	}

	if (ans.data_len == 0 
	|| ans.ret != 0)
	{
		return (ans.ret >= 0 ? ans.ret : -ans.ret_errno);
	}

	char *acl_text = get_buffer(ans.data_len);
		
	if (rfs_receive_data(&instance->sendrecv, acl_text, ans.data_len) == -1)
	{
		free_buffer(acl_text);
		return -ECONNABORTED;
	}
		
	DEBUG("acl: %s\n", acl_text);

	acl_t acl = rfs_acl_from_text(acl_text, 
		(instance->nss.use_nss ? nss_resolve : local_resolve), 
		(void *)instance);

	free_buffer(acl_text);
		
	if (acl == NULL)
	{
		return -EINVAL;
	}

	ssize_t xattr_size = xattr_acl_size(acl, NULL);

	DEBUG("xattr acl size: %lu, available size: %lu\n", (unsigned long)xattr_size, (unsigned long)size);

	if (size == 0)
	{
		return xattr_size;
	}

	if (xattr_size > size)
	{
		return -ERANGE;
	}

	int copy_ret = rfs_acl_to_xattr(acl, value, size);
	if (copy_ret != 0)
	{
		acl_free(acl);
		return copy_ret;
	}

#ifdef RFS_DEBUG
	dump_acl(acl);
	dump(value, size);
#endif
		
	acl_free(acl);
	
	return xattr_size;
}

static char* local_reverse_resolve(acl_tag_t tag, void *id, void *instance_casted)
{
	DEBUG("locally reverse resolving id: %lu\n", id != NULL ? *(unsigned long *)(id) : ACL_UNDEFINED_ID);

	char *name = NULL;
	const char *looked_up_name = NULL;

	switch (tag)
	{
	case ACL_USER:
		looked_up_name = lookup_uid(*(uid_t *)(id));
		break;
	case ACL_GROUP:
		looked_up_name = lookup_gid(*(gid_t *)(id), (uid_t)(-1));
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
	
	acl_t acl = NULL;
	int copy_ret = rfs_acl_from_xattr(value, size, &acl);
	if (copy_ret != 0)
	{
		return copy_ret;
	}

#ifdef RFS_DEBUG
	dump_acl(acl);
#endif

	size_t acl_text_len = 0;
	char *acl_text = rfs_acl_to_text(acl, 
		(instance->nss.use_nss ? nss_reverse_resolve : local_reverse_resolve), 
		(void *)instance, 
		&acl_text_len);

	if (acl_text == NULL)
	{
		acl_free(acl);
		return -EINVAL;
	}

	acl_free(acl);
	
	DEBUG("acl: %s\n", acl_text);

	uint32_t path_len = strlen(path) + 1;
	uint32_t name_len = strlen(name) + 1;
	
	unsigned overall_size = 
	+ sizeof(path_len) 
	+ sizeof(name_len) 
	+ path_len 
	+ name_len
	+ acl_text_len + 1;
	
	char *buffer = get_buffer(overall_size);
	
	pack(acl_text, acl_text_len + 1, 
	pack(name, name_len, 
	pack(path, path_len, 
	pack_32(&name_len, 
	pack_32(&path_len, buffer
	)))));

	free_buffer(acl_text);
	
	struct command cmd = { cmd_setxattr, overall_size };
	
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
	
	if (ans.command != cmd_setxattr 
	|| ans.data_len != 0)
	{
		return cleanup_badmsg(instance, &ans);
	}
	
	return ans.ret == 0 ? 0 : -ans.ret_errno;
}
#else
int operations_acl_c_empty_module_makes_suncc_angry = 0; /* avoid warning about empty module */
#endif /* ACL_OPERATIONS_AVAILABLE */

