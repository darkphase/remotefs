/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef INSTANCE_H
#define INSTANCE_H

/** remotefs instances */

#ifdef WITH_SSL
#include <openssl/ssl.h>
#endif

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
#ifdef WITH_SSL
	int ssl_enabled;
	SSL *ssl_socket;
#endif
#ifdef RFS_DEBUG
	unsigned long bytes_sent;
	unsigned long bytes_recv;
#endif
};

#ifdef WITH_SSL
struct ssl_info
{
	SSL_CTX *ctx;
	char *last_error;
};
#endif

struct id_lookup_info
{
	struct list *uids;
	struct list *gids;
};

void init_sendrecv(struct sendrecv_info *sendrecv);
void init_id_lookup(struct id_lookup_info *id_lookup);

#ifdef WITH_SSL
void init_ssl(struct ssl_info *ssl);
#endif

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* INSTANCE_H */
