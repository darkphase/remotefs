/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/file.h>

#include "config.h"
#include "operations.h"
#include "operations_rfs.h"
#include "buffer.h"
#include "command.h"
#include "sendrecv.h"
#include "keep_alive_client.h"
#include "list.h"
#include "id_lookup.h"
#include "sockets.h"
#include "resume.h"
#include "utils.h"
#include "instance_client.h"
#include "attr_cache.h"
#include "crypt.h"
#ifdef WITH_SSL
#include "ssl.h"
#endif

#ifdef WITH_EXPORTS_LIST
int rfs_list_exports(struct rfs_instance *instance)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}
	
	struct command cmd = { cmd_listexports, 0 };
	
	if (rfs_send_cmd(&instance->sendrecv, &cmd) < 0)
	{
		return -ECONNABORTED;
	}
	
	struct answer ans = { 0 };
	unsigned header_printed = 0;
	unsigned exports_count = 0;
	
	do
	{
		if (rfs_receive_answer(&instance->sendrecv, &ans) < 0)
		{
			return -ECONNABORTED;
		}
		
		if (ans.command != cmd_listexports)
		{
			return cleanup_badmsg(instance, &ans);
		}
		
		if (ans.data_len == 0)
		{
			break;
		}
		
		if (ans.ret != 0)
		{
			return -ans.ret_errno;
		}
		
		++exports_count;
		
		char *buffer = get_buffer(ans.data_len);
		
		if (rfs_receive_data(&instance->sendrecv, buffer, ans.data_len) < 0)
		{
			free_buffer(buffer);
			return -ECONNABORTED;
		}
		
		uint32_t options = OPT_NONE;
		
		const char *path = buffer + 
		unpack_32(&options, buffer, 0);
		
		if (header_printed == 0)
		{
			INFO("%s\n\n", "Server provides the folowing export(s):");
			header_printed = 1;
		}
		
		INFO("%s", path);
		
		if (options != OPT_NONE)
		{
			INFO("%s", " (");
			if (((unsigned)options & OPT_RO) > 0)
			{
				INFO("%s", describe_option(OPT_RO));
			}
			else if (((unsigned)options & OPT_UGO) > 0)
			{
				INFO("%s", describe_option(OPT_UGO));
			}
			INFO("%s\n", ")");
		}
		else
		{
		INFO("%s\n", "");
		}
		
		free_buffer(buffer);
	}
	while (ans.data_len != 0
	&& ans.ret == 0
	&& ans.ret_errno == 0);
	
	if (exports_count == 0)
	{
		INFO("%s\n", "Server provides no exports (this is odd)");
	}
	else
	{
		INFO("%s\n", "");
	}
	
	return 0;
}
#else
int operations_exports_c_empty_module_makes_suncc_angry = 0;
#endif /* WITH_EXPORTS_LIST */

