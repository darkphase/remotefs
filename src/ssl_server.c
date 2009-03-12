/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/
#ifdef WITH_SSL

#include "config.h"
#include "ssl_server.h"

static SSL_METHOD* choose_ssl_server_method()
{
#if ! defined (OPENSSL_NO_SSL2) && ! defined (OPENSSL_NO_SSL3)
	DEBUG("%s\n", "using OpenSSL v2 and v3");
	return SSLv23_server_method();
#elif ! defined (OPENSSL_NO_SSL3)
	DEBUG("%s\n", "using OpenSSL v3");
	return SSLv3_server_method();
#elif ! defined (OPENSSL_NO_SSL2)
	DEBUG("%s\n", "using OpenSSL v2");
	return SSLv2_server_method();
#else
#        error "Not supported OpenSSL version"
#endif
}

SSL* rfs_init_server_ssl(SSL_CTX **ctx, const char *key_file, const char *cert_file, const char *ciphers)
{
	SSL_METHOD *method = choose_ssl_server_method();
	
	return rfs_init_ssl(ctx, method, key_file, cert_file, ciphers);
}

int rfs_accept_ssl(SSL *socket)
{
	DEBUG("%s\n", "trying to accept SSL connection");
	if (SSL_accept(socket) != 1)
	{
		return -1;
	}
	
	return 0;
}

#else
int ssl_server_c_empty_module_makes_suncc_angry = 0; /* avoid warning about empty module */
#endif /* WITH_SSL */

