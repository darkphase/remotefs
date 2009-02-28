/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#if defined FREEBSD
#	include <netinet/in.h>
#	include <sys/uio.h>
#	include <sys/socket.h>
#endif
#if defined QNX
#       include <sys/socket.h>
#endif
#if defined DARWIN
#	include <netinet/in.h>
#	include <sys/uio.h>
#	include <sys/socket.h>
#endif
#ifdef WITH_IPV6
#	include <netdb.h>
#endif
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <utime.h>
#include <pwd.h>
#include <grp.h>

#include "config.h"
#include "server_handlers.h"
#include "command.h"
#include "sendrecv.h"
#include "buffer.h"
#include "exports.h"
#include "list.h"
#include "passwd.h"
#include "inet.h"
#include "keep_alive_server.h"
#include "crypt.h"
#include "path.h"
#include "id_lookup.h"
#include "sockets.h"
#include "cleanup.h"
#include "utils.h"
#include "instance_server.h"
#include "server.h"

int _handle_getnames(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	/* send users */

	size_t users_len = 0;

	struct list *uid = instance->id_lookup.uids;
	while (uid != NULL)
	{
		struct uid_look_ent *entry = (struct uid_look_ent *)uid->data;
		users_len += strlen(entry->name) + 1;

		uid = uid->next;
	}

	struct answer ans = { cmd_getnames, users_len, 0, 0 };

	char *users = get_buffer(users_len);
	size_t written = 0;

	uid = instance->id_lookup.uids;
	while (uid != NULL)
	{	
		struct uid_look_ent *entry = (struct uid_look_ent *)uid->data;

		DEBUG("%s\n", entry->name);

		memcpy(users + written, entry->name, strlen(entry->name) + 1);
		written += strlen(entry->name) + 1;

		uid = uid->next;
	}

	dump(users, users_len);

	if (rfs_send_answer_data(&instance->sendrecv, &ans, users, ans.data_len) < 0)
	{
		free_buffer(users);
		return -1;
	}

	free_buffer(users);

	/* send groups */

	size_t groups_len = 0;

	struct list *gid = instance->id_lookup.gids;
	while (gid != NULL)
	{
		struct gid_look_ent *entry = (struct gid_look_ent *)gid->data;
		groups_len += strlen(entry->name) + 1;

		gid = gid->next;
	}

	ans.data_len = groups_len;

	char *groups = get_buffer(groups_len);
	written = 0;

	gid = instance->id_lookup.gids;
	while (gid != NULL)
	{	
		struct gid_look_ent *entry = (struct gid_look_ent *)gid->data;

		memcpy(groups + written, entry->name, strlen(entry->name) + 1);
		written += strlen(entry->name) + 1;

		gid = gid->next;
	}
	
	dump(groups, groups_len);

	if (rfs_send_answer_data(&instance->sendrecv, &ans, groups, ans.data_len) < 0)
	{
		free_buffer(groups);
		return -1;
	}

	free_buffer(groups);

	return 0;
}

