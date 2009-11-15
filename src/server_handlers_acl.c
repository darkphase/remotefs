#include "options.h"

#if defined ACL_AVAILABLE

#include <errno.h>
#include <string.h>
#include <sys/acl.h>

#include "acl/acl_utils.h"
#include "acl/acl_utils_server.h"
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
	
	acl_t acl = NULL;
	int get_acl_ret = rfs_get_file_acl(path, name, &acl);

	free_buffer(buffer);

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
			free_buffer(acl);
			return reject_request(instance, cmd, ECANCELED) == 0 ? 1 : -1;
		}
	
	acl_free(acl);
	}

	DEBUG("acl text: \n%s\n", acl_text == NULL ? "NULL" : acl_text);
	
	struct answer ans = { cmd_getxattr, acl_text_len + 1, 0, 0 };

	send_token_t token = { 0, {{ 0 }} };
	if (do_send(&instance->sendrecv, 
		queue_data(acl_text == NULL ? "" : acl_text, acl_text == NULL ? 0 : acl_text_len + 1, 
		queue_ans(&ans, &token))) < 0)
	{
		free_buffer(acl_text);
		return -1;
	}
	
	free_buffer(acl_text);
	
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

	const char *path = 
	unpack_32(&name_len, 
	unpack_32(&path_len, buffer
	));
	
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
	
	const char *acl_text = name + name_len;
	
	DEBUG("acl: %s\n", acl_text);
	
	acl_t acl = rfs_acl_from_text(acl_text, 
		id_lookup_resolve, 
		(void *)&instance->id_lookup);
	
	if (acl == NULL)
	{
		free_buffer(buffer);
		return reject_request(instance, cmd, ECANCELED) == 0 ? 1 : -1;
	}

	int set_acl_ret = rfs_set_file_acl(path, name, acl);
	
	free_buffer(buffer);
	acl_free(acl);
		
	struct answer ans = { cmd_setxattr, 0, set_acl_ret == 0 ? 0 : -1, -set_acl_ret };
	return (rfs_send_answer(&instance->sendrecv, &ans) == -1 ? -1 : 0);
}

#else
int server_handlers_acl_c_empty_module_makes_suncc_angry = 0; /* avoid warning about empty module */
#endif /* ACL_AVAILABLE */

