/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef WITH_SSL

#include "config.h"
#include "ssl.h"

static int try_init_ssl(SSL_METHOD *method, const char *keyfile, const char *certfile, const char *ciphers)
{
	int ret = 0;

	SSL_CTX *ssl_ctx = NULL;
	SSL* ssl_sock = rfs_init_ssl(
		&ssl_ctx, 
		method, 
		keyfile, 
		certfile, 
		ciphers);

	if (ssl_sock == NULL
	|| ssl_ctx == NULL)
	{
		ret = -1;
	}
	
	rfs_clear_ssl(&ssl_sock, &ssl_ctx);
	
	return ret;
}

static int check_file(const char *filename)
{
	errno = 0;

	int fd = open(filename, O_RDONLY);
	if (fd == -1)
	{
		return -errno;
	}

	close(fd);
	return 0;
}

int check_ssl(SSL_METHOD *method, const char *keyfile, const char *certfile, const char *ciphers)
{
	DEBUG("%s\n", "checking SSL availability");

	int keycheck = check_file(keyfile);
	if (keycheck != 0)
	{
		WARN("WARNING: Can't open key file at %s: %s\n", keyfile, strerror(-keycheck));
		return -1;
	}

	int certcheck = check_file(certfile);
	if (certcheck != 0)
	{
		WARN("WARNING: Can't open cert file at %s: %s\n", certfile, strerror(-certcheck));
		return -1;
	}

	if (try_init_ssl(method, keyfile, certfile, ciphers) != 0)
	{
		char *ssl_error = rfs_last_ssl_error(NULL);

		WARN("WARNING: Can't init SSL: %s\n", ssl_error);
		INFO("%s\n", "INFO: Please check that SSL key and cert are correct");

		free(ssl_error);

		return -1;
	}

	return 0;
}

#endif /* WITH_SSL */

