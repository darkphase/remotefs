/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "instance_client.h"
#include "operations/operations_rfs.h"

#ifdef WITH_EXPORTS_LIST
int list_exports_main(struct rfs_instance *instance)
{
	int conn_ret = rfs_reconnect(instance, 1, 0);
	if (conn_ret != 0)
	{
		return -conn_ret;
	}
	
	int list_ret = rfs_listexports(instance);
	if (list_ret < 0)
	{
		ERROR("Error listing exports: %s\n", strerror(-list_ret));
	}
	
	rfs_disconnect(instance, 1);
	
	free(instance->config.host);
	
	if (instance->config.path != 0)
	{
		free(instance->config.path);
	}
	
	if (instance->config.auth_passwd != NULL)
	{
		free(instance->config.auth_passwd);
	}
	
	return list_ret;
}
#else
int list_exports_c_empty_module = 0;
#endif

