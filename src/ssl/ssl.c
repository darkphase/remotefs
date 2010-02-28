/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/
#ifdef WITH_SSL

#include <string.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include "ssl.h"
#include "../buffer.h"
#include "../config.h"

SSL* rfs_init_ssl(
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
	/* TODO: check SSL_ERROR_WANT_WRITE */
	return SSL_write(socket, (void *)buffer, (int)size);
}

int rfs_ssl_read(SSL *socket, char *buffer, size_t size)
{
	/* TODO: check SSL_ERROR_WANT_READ */
	return SSL_read(socket, (void *)buffer, (int)size);
}

#else
int ssl_c_empty_module_makes_suncc_angry = 0; /* avoid warning about empty module */
#endif /* WITH_SSL */

