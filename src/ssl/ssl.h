/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef SSL_H
#define SSL_H

#ifdef WITH_SSL

/** common SSL related routines */

#include <openssl/ssl.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/** init SSL and CTX using specified certificates */
SSL* rfs_init_ssl(
	SSL_CTX **ctx, 
	SSL_METHOD *method, 
	const char *key_file, 
	const char *cert_file, 
	const char *cipher_list);

/** attach SSL structures to socket */
int rfs_attach_ssl(SSL *ssl_socket, int socket);

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

#endif /* WITH_SSL */

#endif /* SSL_H */

