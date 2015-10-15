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

struct rfs_list;

typedef struct
{
	int socket;
	unsigned connection_lost;
	unsigned oob_received; /* used by client only */
	long connect_timeout;
	long recv_timeout;
	long send_timeout;
#ifdef RFS_DEBUG
	unsigned long bytes_sent;
	unsigned long bytes_recv;
	unsigned long recv_interrupts;
	unsigned long send_interrupts;
	unsigned long recv_timeouts;
	unsigned long send_timeouts;
	unsigned long conn_timeouts;
#endif
} rfs_sendrecv_info_t;

typedef struct
{
	struct rfs_list *uids;
	struct rfs_list *gids;
} rfs_id_lookup_info_t;

void init_sendrecv(rfs_sendrecv_info_t *sendrecv);
void init_id_lookup(rfs_id_lookup_info_t *id_lookup);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* INSTANCE_H */
