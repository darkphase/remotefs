/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifdef WITH_SSL

#ifndef SSL_H
#define SSL_H

/** SSL related routines */

#include <openssl/ssl.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/** init SSL for client */
SSL* rfs_init_client_ssl(SSL_CTX **ctx, const char *key_file, const char *cert_file, const char *ciphers);

/** init SSL for server */
SSL* rfs_init_server_ssl(SSL_CTX **ctx, const char *key_file, const char *cert_file, const char *ciphers);

/** attach SSL structures to socket */
int rfs_attach_ssl(SSL *ssl_socket, int socket);

/** accept SSLed connection */
int rfs_accept_ssl(SSL *socket);

/** connect SSLed socket */
int rfs_connect_ssl(SSL *socket);

/** cleanup allocated SSL structures */
int rfs_clear_ssl(SSL **socket, SSL_CTX **ctx);

/** get last error reported by OpenSSL, prev_error will be free()d if not NULL */
char* rfs_last_ssl_error(char *prev_error);

/** write data to socket using SSL */
int rfs_ssl_write(SSL *socket, const char *buffer, size_t size);

/** read data from socket using SSL */
int rfs_ssl_read(SSL *socket, char *buffer, size_t size);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* SSL_H */

#endif /* WITH_SSL */
