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
	char *acl_text = rfs_acl_to_text(acl, local_reverse_resolve, (void *)instance, &acl_text_len);

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
	
	char *buffer = malloc(overall_size);
	
	pack(acl_text, acl_text_len + 1, 
	pack(name, name_len, 
	pack(path, path_len, 
	pack_32(&name_len, 
	pack_32(&path_len, buffer
	)))));

	free(acl_text);
	
	struct command cmd = { cmd_setxattr, overall_size };
	
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
	
	if (ans.command != cmd_setxattr 
	|| ans.data_len != 0)
	{
		return cleanup_badmsg(instance, &ans);
	}
	
	return ans.ret == 0 ? 0 : -ans.ret_errno;
}
#else
int setxattr_c_empty_module_makes_suncc_angry = 0; /* avoid warning about empty module */
#endif /* ACL_OPERATIONS_AVAILABLE */
