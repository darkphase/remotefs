/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "../../buffer.h"
#include "../../command.h"
#include "../../config.h"
#include "../../exports.h"
#include "../../handling.h"
#include "../../instance_server.h"
#include "../../list.h"
#include "../../sendrecv_server.h"

#ifdef WITH_EXPORTS_LIST
int _handle_listexports(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct rfs_command *cmd)
{
	const struct rfs_list *export_node = instance->exports.list;
	if (export_node == NULL)
	{
		return reject_request(instance, cmd, ECANCELED);
	}
	
	while (export_node != NULL)
	{
		const struct rfs_export *export_rec = (const struct rfs_export *)export_node->data;
		
		unsigned path_len = strlen(export_rec->path) + 1;
		uint32_t options = export_rec->options;
		
		unsigned overall_size = sizeof(options) + path_len;
		
		char *buffer = malloc(overall_size);
		
		pack(export_rec->path, path_len, 
		pack_32(&options, buffer
		));

		struct rfs_answer ans = { cmd_listexports, overall_size, 0, 0 };
		
		send_token_t token = { 0 };
		if (do_send(&instance->sendrecv, 
			queue_data(buffer, overall_size, 
			queue_ans(&ans, &token))) < 0)
		{
			free(buffer);
			return -1;
		}
		
		free(buffer);
		
		export_node = export_node->next;
	}
	
	struct rfs_answer last_ans = { cmd_listexports, 0, 0, 0 };	
	return (rfs_send_answer(&instance->sendrecv, &last_ans) == -1 ? -1 : 0);
}
#else
int listexports_c_empty_module_makes_suncc_angry = 0;
#endif /* WITH_EXPORTS_LIST */
