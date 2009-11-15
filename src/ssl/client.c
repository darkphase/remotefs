/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/
#ifdef WITH_SSL

#include <stdlib.h>
#include <string.h>

#include "client.h"
#include "../buffer.h"
#include "../config.h"

SSL_METHOD* choose_ssl_client_method()
{
#if ! defined (OPENSSL_NO_SSL2) && ! defined (OPENSSL_NO_SSL3)
	DEBUG("%s\n", "using OpenSSL v2 and v3");
	return SSLv23_client_method();
#elif ! defined (OPENSSL_NO_SSL3)
	DEBUG("%s\n", "using OpenSSL v3");
	return SSLv3_client_method();
#elif ! defined (OPENSSL_NO_SSL2)
	DEBUG("%s\n", "using OpenSSL v2");
	return SSLv2_client_method();
#else
#        error "Not supported OpenSSL version"
#endif
}

SSL* rfs_init_client_ssl(SSL_CTX **ctx, const char *key_file, const char *cert_file, const char *cipher_list)
{
	SSL_METHOD *method = choose_ssl_client_method();
	SSL* ret = rfs_init_ssl(ctx, method, key_file, cert_file, cipher_list);

	return ret;
}

int rfs_connect_ssl(SSL *socket)
{
	DEBUG("%s\n", "trying to connect with SSL");
	if (SSL_connect(socket) != 1)
	{
		return -1;
	}
	
	return 0;
}

#else
int ssl_client_c_empty_module_makes_suncc_angry = 0; /* avoid warning about empty module */
#endif /* WITH_SSL */

