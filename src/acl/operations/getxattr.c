/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include "../../options.h"

#ifdef ACL_OPERATIONS_AVAILABLE

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/acl.h>

#include "../xattr_linux.h"
#include "../local_resolve.h"
#include "../utils.h"
#include "../../buffer.h"
#include "../../config.h"
#include "../../command.h"
#include "../../id_lookup_client.h"
#include "../../instance_client.h"
#include "../../operations/utils.h"
#include "../../sendrecv_client.h"
#include "../libacl/include/acl_ea.h"

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
	
	char *buffer = malloc(overall_size);
	
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
		free(buffer);
		return -ECONNABORTED;
	}
	
	free(buffer);
	
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

	char *acl_text = malloc(ans.data_len);
		
	if (rfs_receive_data(&instance->sendrecv, acl_text, ans.data_len) == -1)
	{
		free(acl_text);
		return -ECONNABORTED;
	}
		
	DEBUG("acl: %s\n", acl_text);

	acl_t acl = rfs_acl_from_text(acl_text, local_resolve, (void *)instance);

	free(acl_text);
		
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

#else
int getxattr_c_empty_module_makes_suncc_angry = 0; /* avoid warning about empty module */
#endif /* ACL_OPERATIONS_AVAILABLE */
