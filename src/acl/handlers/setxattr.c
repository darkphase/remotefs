/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include "../../options.h"

#if defined ACL_AVAILABLE

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/acl.h>

#include "../../buffer.h"
#include "../../command.h"
#include "../../config.h"
#include "../../instance_server.h"
#include "../../sendrecv_server.h"
#include "../../server.h"
#include "../id_lookup_resolve.h"
#include "../server.h"
#include "../utils.h"

int _handle_setxattr(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *buffer = malloc(cmd->data_len);
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (rfs_receive_data(&instance->sendrecv, buffer, cmd->data_len) == -1)
	{
		free(buffer);
		return -1;
	}
	
	uint32_t path_len = 0;
	uint32_t name_len = 0;

	const char *path = 
	unpack_32(&name_len, 
	unpack_32(&path_len, buffer
	));
	
	if (strlen(path) + 1 != path_len)
	{
		free(buffer);
		return reject_request(instance, cmd, EBADMSG) == 0 ? 1 : -1;
	}
	
	DEBUG("path: %s\n", path);
	
	const char *name = path + path_len;
	
	if (strlen(name) + 1 != name_len)
	{
		free(buffer);
		return reject_request(instance, cmd, EBADMSG) == 0 ? 1 : -1;
	}
	
	DEBUG("name: %s\n", name);
	
	const char *acl_text = name + name_len;
	
	DEBUG("acl: %s\n", acl_text);
	
	acl_t acl = rfs_acl_from_text(acl_text, 
		id_lookup_resolve, 
		(void *)&instance->id_lookup);
	
	if (acl == NULL)
	{
		free(buffer);
		return reject_request(instance, cmd, ECANCELED) == 0 ? 1 : -1;
	}

	int set_acl_ret = rfs_set_file_acl(path, name, acl);
	
	free(buffer);
	acl_free(acl);
		
	struct answer ans = { cmd_setxattr, 0, set_acl_ret == 0 ? 0 : -1, -set_acl_ret };
	return (rfs_send_answer(&instance->sendrecv, &ans) == -1 ? -1 : 0);
}

#else
int server_handlers_acl_setxattr_c_empty_module_makes_suncc_angry = 0; /* avoid warning about empty module */
#endif /* ACL_AVAILABLE */

