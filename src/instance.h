/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef INSTANCE_H
#define INSTANCE_H

/** remotefs instances */

#include "config.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct list;

struct sendrecv_info
{
	int socket;
	unsigned connection_lost;
	unsigned oob_received; /* used by client only */
#ifdef RFS_DEBUG
	unsigned long bytes_sent;
	unsigned long bytes_recv;
#endif
};

struct id_lookup_info
{
	struct list *uids;
	struct list *gids;
};

void init_sendrecv(struct sendrecv_info *sendrecv);
void init_id_lookup(struct id_lookup_info *id_lookup);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* INSTANCE_H */
