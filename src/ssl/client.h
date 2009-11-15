/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef SSL_CLIENT_H
#define SSL_CLIENT_H

#ifdef WITH_SSL

/** client's SSL related routines */

#include "ssl.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/** init SSL for client */
SSL* rfs_init_client_ssl(SSL_CTX **ctx, const char *key_file, const char *cert_file, const char *ciphers);

/** connect SSLed socket */
int rfs_connect_ssl(SSL *socket);

/** choose SSL method (v23, v3, v2) */
SSL_METHOD* choose_ssl_client_method();

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* WITH_SSL */

#endif /* SSL_CLIENT_H */

