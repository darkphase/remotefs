/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef SUG_COMMON_H
#define SUG_COMMON_H

#ifdef WITH_SSL

#include "ssl/ssl.h"

/** return 0 if SSL cna be initialized with passed key/cert/ciphers */
int check_ssl(SSL_METHOD *method, const char *keyfile, const char *certfile, const char *ciphers);

#endif /* WITH_SSL */

#endif /* SUG_COMMON_H */

