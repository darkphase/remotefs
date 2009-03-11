/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/
#ifdef WITH_SSL

#if defined DARWIN || defined QNX
#       include <string.h>
#endif
#include <openssl/err.h>
#include <openssl/rand.h>

#include "buffer.h"
#include "config.h"
#include "ssl.h"

static SSL* rfs_init_ssl(
	SSL_CTX **ctx, 
	SSL_METHOD *method, 
	const char *key_file, 
	const char *cert_file, 
	const char *cipher_list);

static SSL_METHOD* choose_ssl_client_method()
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

SSL* rfs_init_client_ssl(SSL_CTX **ctx, const char *key_file, const char *cert_file, const char *cipher_list)
{
	SSL_METHOD *method = choose_ssl_client_method();

#ifndef RFS_DEBUG
	char *home_dir = getenv("HOME");
#else
	char *home_dir = ".";
#endif
	char *key = NULL;
	char *cert = NULL;

	if (home_dir == NULL)
	{
		return NULL;
	}
	
	size_t key_path_size = strlen(home_dir)+ strlen(key_file) + 2; /* 2 == '/' + '\0' */
	key = get_buffer(key_path_size);
	if (key == NULL)
	{
		return NULL;
	}
	snprintf(key, key_path_size, "%s/%s", home_dir, key_file);
	
	size_t cert_path_size = strlen(home_dir) + strlen(cert_file) + 2;
	cert = get_buffer(cert_path_size);
	if (cert == NULL)
	{
		return NULL;
	}
	snprintf(cert, cert_path_size, "%s/%s", home_dir, cert_file);
	
	SSL* ret = rfs_init_ssl(ctx, method, key, cert, cipher_list);

	if (key != NULL)
	{
		free_buffer(key);
	}
	
	if (cert != NULL)
	{
		free_buffer(cert);
	}

	return ret;
}

SSL* rfs_init_server_ssl(SSL_CTX **ctx, const char *key_file, const char *cert_file, const char *ciphers)
{
	SSL_METHOD *method = choose_ssl_server_method();
	
	return rfs_init_ssl(ctx, method, key_file, cert_file, ciphers);
}

static SSL* rfs_init_ssl(
	SSL_CTX **ctx,
	SSL_METHOD *method, 
	const char *key_file, 
	const char *cert_file, 
	const char *cipher_list)
{
	srand(time(NULL));
	
	int count = (rand() % 100) + 50;
	int i = 0; for (i = 0; i < count; ++i)
	{
		int ch = rand();
		RAND_seed(&ch, sizeof(ch));
	}

	DEBUG("%s\n", "initing SSL");
	
	SSL_library_init();
	SSL_load_error_strings();
	
	*ctx = SSL_CTX_new(method);
	if (*ctx == NULL)
	{
		return NULL;
	}
	
	DEBUG("loading private key: %s\n", key_file);
	if (SSL_CTX_use_PrivateKey_file(*ctx, key_file, SSL_FILETYPE_PEM) != 1)
	{
		goto ctx_error;
	}
	
	DEBUG("loading certificate: %s\n", cert_file);
	if (SSL_CTX_use_certificate_file(*ctx, cert_file, SSL_FILETYPE_PEM) != 1)
	{
		goto ctx_error;
	}
	
	SSL_CTX_set_verify(*ctx, SSL_VERIFY_NONE, NULL);
	
	DEBUG("chiphers list: %s\n", cipher_list != NULL ? cipher_list : "NULL" );
	if ( cipher_list != NULL )
	{
		if (SSL_CTX_set_cipher_list(*ctx, cipher_list) != 1)
		{
			goto ctx_error;
		}
	}
	
	SSL *ssl = SSL_new(*ctx);
	if (ssl == NULL)
	{
		goto ctx_error;
	}

	goto ok;

ctx_error:
	SSL_CTX_free(*ctx);
	*ctx = NULL;
	return NULL;

ok:
	SSL_set_read_ahead(ssl, 1);
	return ssl;
}

int rfs_clear_ssl(SSL **socket, SSL_CTX **ctx)
{
	DEBUG("%s\n", "clearing SSL");
	
	if (socket != NULL 
	&& *socket != NULL)
	{
		if (SSL_shutdown(*socket) == 0)
		{
			SSL_shutdown(*socket);
		}
		
		SSL_clear(*socket);
		SSL_free(*socket);
		
		*socket = NULL;
	}
	
	if (ctx != NULL 
	&& *ctx != NULL)
	{
		SSL_CTX_free(*ctx);
		
		*ctx = NULL;
	}
	
	return 0;
}

int rfs_attach_ssl(SSL *ssl_socket, int socket)
{
	DEBUG("attaching SSL to socket %d\n", socket);
	SSL_set_fd(ssl_socket, socket);
	
	return 0;
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

int rfs_connect_ssl(SSL *socket)
{
	DEBUG("%s\n", "trying to connect with SSL");
	if (SSL_connect(socket) != 1)
	{
		return -1;
	}
	
	return 0;
}

char* rfs_last_ssl_error(char *prev_error)
{
	unsigned long ret = ERR_peek_error();
	if (ret == 0)
	{
		return "No SSL error in queue. Please report this bug.";
	}
	
	if (prev_error != NULL)
	{
		free(prev_error);
	}
	
	char *last_error = strdup(ERR_reason_error_string(ret));
	
	ERR_get_error(); /* remove error from stack */
	
	return last_error;
}

int rfs_ssl_write(SSL *socket, const char *buffer, size_t size)
{
	return SSL_write(socket, (void *)buffer, (int)size);
}

int rfs_ssl_read(SSL *socket, char *buffer, size_t size)
{
	return SSL_read(socket, (void *)buffer, (int)size);
}

#else
int ssl_c_empty_module_makes_suncc_angry = 0; /* avoid warning about empty module */
#endif /* WITH_SSL */
