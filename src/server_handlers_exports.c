/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <string.h>

#include "buffer.h"
#include "command.h"
#include "config.h"
#include "exports.h"
#include "instance_server.h"
#include "list.h"
#include "sendrecv.h"
#include "server.h"

#ifdef WITH_EXPORTS_LIST
int _handle_listexports(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	const struct list *export_node = instance->exports.list;
	if (export_node == NULL)
	{
		return reject_request(instance, cmd, ECANCELED);
	}
	
	struct answer ans = { cmd_listexports, 0, 0, 0 };
	
	while (export_node != NULL)
	{
		const struct rfs_export *export_rec = (const struct rfs_export *)export_node->data;
		
		unsigned path_len = strlen(export_rec->path) + 1;
		uint32_t options = export_rec->options;
		
		unsigned overall_size = sizeof(options) + path_len;
		
		char *buffer = get_buffer(overall_size);
		
		pack(export_rec->path, path_len, buffer, 
		pack_32(&options, buffer, 0
		));
		
		ans.data_len = overall_size;
		
		if (rfs_send_answer_data(&instance->sendrecv, &ans, buffer) < 0)
		{
			free_buffer(buffer);
			return -1;
		}
		
		free_buffer(buffer);
		
		export_node = export_node->next;
	}
	
	ans.data_len = 0;
	
	return (rfs_send_answer(&instance->sendrecv, &ans) < 0 ? -1 : 0);
}
#else
int server_handlers_exports_c_empty_module_makes_suncc_angry = 0;
#endif /* WITH_EXPORTS_LIST */

