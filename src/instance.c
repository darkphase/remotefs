/*
remotefs file system
See the file AUTHORS for copyright information.

This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include "config.h"
#include "instance.h"

void init_sendrecv(rfs_sendrecv_info_t *sendrecv)
{
	sendrecv->socket = -1;
	sendrecv->oob_received = 0;
	sendrecv->connection_lost = 1; /* yes, it should be set to 0 on connect or accept */
	sendrecv->connect_timeout = DEFAULT_CONNECT_TIMEOUT;
	sendrecv->recv_timeout = DEFAULT_RECV_TIMEOUT;
	sendrecv->send_timeout = DEFAULT_SEND_TIMEOUT;
#ifdef RFS_DEBUG
	sendrecv->bytes_sent = 0;
	sendrecv->bytes_recv = 0;
	sendrecv->recv_interrupts = 0;
	sendrecv->send_interrupts = 0;
	sendrecv->recv_timeouts = 0;
	sendrecv->send_timeouts = 0;
	sendrecv->conn_timeouts = 0;
#endif
}

void init_id_lookup(rfs_id_lookup_info_t *id_lookup)
{
	id_lookup->uids = NULL;
	id_lookup->gids = NULL;
}
