/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/
#if defined SOLARIS
#    include <nss_common.h>
#    include <nss_dbdefs.h>
     typedef nss_status_t NSS_RET;
#    define NSS_ARGS(args) ((nss_XbyY_args_t*)args)
#    define NSS_STATUS_UNAVAIL  NSS_UNAVAIL
#    define NSS_STATUS_SUCCESS  NSS_SUCCESS
#    define NSS_STATUS_TRYAGAIN NSS_TRYAGAIN
#    define NSS_STATUS_NOTFOUND NSS_NOTFOUND

#elif defined FREEBSD
#    include <nss.h>
#    define NSS_RET enum nss_status

#elif defined LINUX
#    include <nss.h>
     typedef enum nss_status NSS_RET;

#else
#    error "Your OS is not supported"
#endif

#include <stdlib.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "client.h"
#include "client_common.h"
#include "client_ent.h"
#include "client_for_server.h"
#include "config.h"

static inline char* even_addr(char *address)
{
	size_t mod = ((size_t)address % sizeof(void *));

	return (mod == 0 ? address : address + (sizeof(void *) - mod));
}

static inline size_t even_size(size_t size)
{
	size_t mod = (size % sizeof(void *));
	return (mod == 0 ? size : size + (sizeof(void *) - mod));
}

static inline int build_pwd(struct passwd *result, char *buffer, size_t buflen, const struct passwd *nss_ret)
{
	if (nss_ret == NULL)
	{
		return -ENOENT;
	}

	size_t name_len = (nss_ret->pw_name == NULL ? sizeof("") : strlen(nss_ret->pw_name) + 1);
	size_t overall_len = even_size(name_len * 2)
		+ even_size(sizeof(DEFAULT_SHELL)) 
		+ even_size(sizeof(DEFAULT_HOME)) 
		+ even_size(sizeof(DEFAULT_PASSWD));

	if (buffer == NULL
	|| overall_len > buflen)
	{
		return -ERANGE;
	}

	result->pw_uid = nss_ret->pw_uid;
	result->pw_gid = (gid_t)(nss_ret->pw_uid);

	if (nss_ret->pw_gid != (gid_t)-1)
	{
		result->pw_gid = nss_ret->pw_gid;
	}

	if (nss_ret->pw_name != NULL)
	{
		memcpy(buffer, nss_ret->pw_name, name_len);
		memcpy(buffer + name_len, nss_ret->pw_name, name_len);
		result->pw_name = buffer;
		result->pw_gecos = buffer + name_len;
	}
	else
	{
		/* empty username and real name */
		buffer[0] = 0;
		buffer[1] = 0;
		result->pw_name = buffer;
		result->pw_gecos = buffer + 1;
	}

	result->pw_passwd = memcpy(even_addr(buffer + name_len * 2), DEFAULT_PASSWD, sizeof(DEFAULT_PASSWD));
	result->pw_dir = memcpy(even_addr(result->pw_passwd + sizeof(DEFAULT_PASSWD)), DEFAULT_HOME, sizeof(DEFAULT_HOME));
	result->pw_shell = memcpy(even_addr(result->pw_dir + sizeof(DEFAULT_HOME)), DEFAULT_SHELL, sizeof(DEFAULT_SHELL));

	return 0;
}

static inline int build_grp(struct group *result, char *buffer, size_t buflen, const struct group *nss_ret)
{
	if (nss_ret == NULL)
	{
		return -ENOENT;
	}

	size_t name_len = (nss_ret->gr_name == NULL ? sizeof("") : strlen(nss_ret->gr_name) + 1);
	size_t overall_len = even_size(name_len) 
		+ even_size(sizeof(DEFAULT_PASSWD)) 
		+ even_size(1); /* this is for gr_mem, which is always NULL */

	if (buffer == NULL 
	|| overall_len > buflen)
	{
		return -ERANGE;
	}

	result->gr_gid = nss_ret->gr_gid;

	if (nss_ret->gr_name != NULL)
	{
		memcpy(buffer, nss_ret->gr_name, name_len);
		result->gr_name = buffer;
	}
	else
	{
		/* empty name */
		buffer[0] = 0;
		result->gr_name = buffer;
	}

	result->gr_passwd = memcpy(even_addr(buffer + name_len), DEFAULT_PASSWD, sizeof(DEFAULT_PASSWD));
	*result->gr_mem = (even_addr(result->gr_passwd + sizeof(DEFAULT_PASSWD)));
	*result->gr_mem = NULL;

	return 0;
}

static inline unsigned libnss_avail()
{
	if (rfsnss_is_server_running(getuid()) == 0 
	&& rfsnss_is_server_running((uid_t)-1) == 0)
	{
		return 0;
	}

	return 1;
}

static inline NSS_RET make_ret(const int *errnop, const void *result)
{
	if (*errnop == ERANGE)
	{
		return NSS_STATUS_TRYAGAIN;
	}

	return (result != NULL ? NSS_STATUS_SUCCESS : NSS_STATUS_NOTFOUND);
}

NSS_RET _nss_rfs_getpwnam_r(const char *pwnam, 
	struct passwd *result, 
	char *buffer, 
	size_t buflen, 
	int *errnop)
{
	if (libnss_avail() == 0)
	{
		return NSS_STATUS_UNAVAIL;
	}

	struct passwd *nss_ret = rfsnss_getpwnam(pwnam);
	*errnop = -build_pwd(result, buffer, buflen, nss_ret);

	return make_ret(errnop, nss_ret);
}

NSS_RET _nss_rfs_getpwuid_r(uid_t uid,
	struct passwd *result,
	char *buffer,
	size_t buflen,
	int *errnop)
{
	if (libnss_avail() == 0)
	{
		return NSS_STATUS_UNAVAIL;
	}

	struct passwd *nss_ret = rfsnss_getpwuid(uid);
	*errnop = -build_pwd(result, buffer, buflen, nss_ret);

	return make_ret(errnop, nss_ret);
}

NSS_RET _nss_rfs_getgrnam_r(const char *grnam,
	struct group *result,
	char *buffer,
	size_t buflen,
	int *errnop)
{
	if (libnss_avail() == 0)
	{
		return NSS_STATUS_UNAVAIL;
	}

	struct group *nss_ret = rfsnss_getgrnam(grnam);
	*errnop = -build_grp(result, buffer, buflen, nss_ret);

	return make_ret(errnop, nss_ret);
}

NSS_RET _nss_rfs_getgrgid_r(gid_t gid,
	struct group *result,
	char *buffer,
	size_t buflen,
	int *errnop)
{
	if (libnss_avail() == 0)
	{
		return NSS_STATUS_UNAVAIL;
	}

	struct group *nss_ret = rfsnss_getgrgid(gid);
	*errnop = -build_grp(result, buffer, buflen, nss_ret);

	return make_ret(errnop, nss_ret);
}

NSS_RET _nss_rfs_setpwent(void)
{
	if (libnss_avail() == 0)
	{
		return NSS_STATUS_UNAVAIL;
	}

	rfsnss_setpwent();

	return NSS_STATUS_SUCCESS;
}

NSS_RET _nss_rfs_getpwent_r(struct passwd *result,
	char *buffer,
	size_t buflen,
	int *errnop)
{
	if (libnss_avail() == 0)
	{
		return NSS_STATUS_UNAVAIL;
	}
	
	struct passwd *nss_ret = rfsnss_getpwent();
	*errnop = -build_pwd(result, buffer, buflen, nss_ret);

	return make_ret(errnop, nss_ret);
}

NSS_RET _nss_rfs_endpwent(void)
{
	if (libnss_avail() == 0)
	{
		return NSS_STATUS_UNAVAIL;
	}

	rfsnss_endpwent();

    return NSS_STATUS_SUCCESS;
}

NSS_RET _nss_rfs_setgrent(void)
{
	if (libnss_avail() == 0)
	{
		return NSS_STATUS_UNAVAIL;
	}

	rfsnss_setgrent();

	return NSS_STATUS_SUCCESS;
}

NSS_RET _nss_rfs_getgrent_r(struct group *result,
	char *buffer,
	size_t buflen,
	int *errnop)
{
	if (libnss_avail() == 0)
	{
		return NSS_STATUS_UNAVAIL;
	}
	
	struct group *nss_ret = rfsnss_getgrent();
	*errnop = -build_grp(result, buffer, buflen, nss_ret);

	return make_ret(errnop, nss_ret);
}

NSS_RET _nss_rfs_endgrent(void)
{
	if (libnss_avail() == 0)
	{
		return NSS_STATUS_UNAVAIL;
	}

	rfsnss_endgrent();

    return NSS_STATUS_SUCCESS;
}

#ifdef SOLARIS
#include "rfs_nss_solaris.c"
#elif defined FREEBSD
#include "rfs_nss_freebsd.c"
#endif

