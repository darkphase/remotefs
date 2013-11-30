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
#include "../../handling.h"
#include "../../instance_server.h"
#include "../../sendrecv_server.h"
#include "../id_lookup_resolve.h"
#include "../server.h"
#include "../utils.h"

int _handle_getxattr(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct rfs_command *cmd)
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
	uint64_t value_size = 0;

	const char *path = 
	unpack_64(&value_size, 
	unpack_32(&name_len, 
	unpack_32(&path_len, buffer
	)));
	
	DEBUG("value size: %lld\n", (long long)value_size);
	
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
	
	acl_t acl = NULL;
	int get_acl_ret = rfs_get_file_acl(path, name, &acl);

	free(buffer);

	if (get_acl_ret != 0)
	{
		acl_free(acl);
		return reject_request(instance, cmd, -get_acl_ret) == 0 ? 1 : -1;
	}

	size_t acl_text_len = 0;
	char *acl_text = NULL;

	if (acl != NULL)
	{
		acl_text = rfs_acl_to_text(acl, 
			id_lookup_reverse_resolve, 
			(void *)&instance->id_lookup, 
			&acl_text_len);

		if (acl_text == NULL)
		{
			free(acl);
			return reject_request(instance, cmd, ECANCELED) == 0 ? 1 : -1;
		}
	
		acl_free(acl);
	}
	else
	{
		acl_text_len = 1; /* empty string */
	}

	DEBUG("acl text len: %llu\n", (long long unsigned)acl_text_len);
	DEBUG("acl text: \n%s\n", acl_text != NULL ? acl_text : "");
	
	struct rfs_answer ans = { cmd_getxattr, acl_text_len, 0, 0 };

	send_token_t token = { 0 };
	if (do_send(&instance->sendrecv, 
		queue_data(acl_text != NULL ? acl_text : "", acl_text_len, 
		queue_ans(&ans, &token))) < 0)
	{
		free(acl_text);
		return -1;
	}
	
	free(acl_text);
	
	return 0;
}

#else
int server_handlers_acl_getxattr__c_empty_module_makes_suncc_angry = 0; /* avoid warning about empty module */
#endif /* ACL_AVAILABLE */
