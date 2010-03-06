/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <string.h>

#include "config.h"
#include "crypt.h"
#include "md5crypt/crypt_md5.h"

char *passwd_hash(const char *password, const char *salt)
{
	char *passwd = md5_crypt(password, salt);

	if (passwd == NULL)
	{
		return 0;
	}
	
	passwd = strdup(passwd + strlen(salt) + 1);
	return passwd;
}
