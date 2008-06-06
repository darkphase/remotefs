#include "crypt.h"

#include <crypt.h>
#include <string.h>

#include "config.h"

char *passwd_hash(const char *password)
{
	const char salt[] = "$1$";
	char *passwd = crypt(password, salt);
	
	if (passwd == NULL)
	{
		return 0;
	}
	
	passwd = strdup(passwd + sizeof(salt));
	return passwd;
}
