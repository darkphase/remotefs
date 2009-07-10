/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef SSL_SERVER_H
#define SSL_SERVER_H

#ifdef WITH_SSL

/** server's SSL related routines */

#include "ssl.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/** init SSL for server */
SSL* rfs_init_server_ssl(SSL_CTX **ctx, const char *key_file, const char *cert_file, const char *ciphers);

/** accept SSLed connection */
int rfs_accept_ssl(SSL *socket);

/** return supported SSL method (v23, v3, v2) */
SSL_METHOD* choose_ssl_server_method();

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* WITH_SSL */

#endif /* SSL_SERVER_H */

