/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <sys/un.h>
#include <string.h>
#include <stdlib.h>

/* the values an types used by nss are not the same on all OS
 * so we must have defines and type common for all systems
 */
#if defined SOLARIS
#    include <nss_common.h>
#    include <nss_dbdefs.h>
#    include "rfs_nss.h"
     typedef nss_status_t NSS_STATUS;
#    define NSS_ARGS(args) ((nss_XbyY_args_t*)args)
#    define NSS_STATUS_UNAVAIL  NSS_UNAVAIL
#    define NSS_STATUS_SUCCESS  NSS_SUCCESS
#    define NSS_STATUS_TRYAGAIN NSS_TRYAGAIN
#    define NSS_STATUS_NOTFOUND NSS_NOTFOUND

#elif defined FREEBSD
#    include <nss.h>
#    include "rfs_nss.h"
#    define NSS_STATUS enum nss_status

#elif defined linux
#    include <nss.h>
#    include "rfs_nss.h"
#    define NSS_STATUS enum nss_status

#else
#    error "Your OS is not supported"
#endif

/* We must fill a passed eg a group structure and the pointer reference
 + a location within a buffer, the pointer must start at correct addresses
 * we calculate the referenced position within the buffer with this macro
 */
 
#define SETPOS(pos,len) { int modulo = ((len + sizeof(char*)) % sizeof(char*)); \
                          if ( modulo ) \
                              pos += len + sizeof(char*) - modulo; \
                          else \
                              pos += len; \
                         }

/**********************************************************
 *
 * query_server()
 *
 * Ask the server for the user/group name or the uid/gid
 *
 * If the server is not running we will return NSS_STATUS_UNAVAIL
 * and NSS_STATUS_SUCCESS or NSS_STATUS_NOTFOUND according
 * to the data returnrd by the server.
 *
 **********************************************************/
 
static NSS_STATUS query_server(cmd_e cmd, char *name, uid_t *uid, int *error)
{
    /* connect with the rfs clients */
    int ret;
    int sock;
    NSS_STATUS nss_state = NSS_STATUS_SUCCESS;
    cmd_t command;
    *error = 0;
    sock = socket(PF_UNIX, SOCK_STREAM, 0);
    if (sock == -1)
    {
         *error = errno;
         perror("connect");
         return NSS_STATUS_UNAVAIL;
    }
    struct sockaddr_un sock_address;
    memset(&sock_address, 0, sizeof(struct sockaddr_un));
    strcpy(sock_address.sun_path, SOCKNAME);
    sock_address.sun_family = AF_UNIX;

    ret = connect(sock, (struct sockaddr*)&sock_address, sizeof(struct sockaddr_un));
    if ( ret == -1 )
    {
         *error = errno;
         perror("connect");
         close(sock);
         return NSS_STATUS_UNAVAIL;
    }
    else
    {
       command.cmd = cmd;
       switch(cmd)
       {
           case GETPWNAM:
           case GETGRNAM:
               strncpy(command.name, name, RFS_LOGIN_NAME_MAX);
               command.name[RFS_LOGIN_NAME_MAX] = '\0';
           break;
           case GETPWUID:
           case GETGRGID:
               command.id = *uid;
           break;
           default:
               return NSS_STATUS_UNAVAIL;
       }

       ret = send(sock, &command, sizeof(command), 0);
       if ( ret == -1 )
       {
           *error = errno;
           perror("send");
           nss_state = NSS_STATUS_UNAVAIL;
       }
       else
       {
           ret = recv(sock, &command, sizeof(command), 0);
           if ( ret == -1 )
           {
               *error = errno;
               perror("recv");
           }
           else if ( command.found == 0 )
           {
               nss_state = NSS_STATUS_NOTFOUND;
           }
           else
           {
               switch(cmd)
               {
                   case GETPWNAM:
                   case GETGRNAM:
                       *uid = command.id;
                   break;
                   case GETPWUID:
                   case GETGRGID:
                       strncpy(name, command.name, RFS_LOGIN_NAME_MAX);
                       name[RFS_LOGIN_NAME_MAX] = '\0';
                   break;
                   default:
                       nss_state = NSS_STATUS_UNAVAIL;
               }
           }
       }
    }

    close(sock);
    return nss_state;
}

/**********************************************************
 *
 * build_pwd()
 * Fill a passwd structur with the user name and it uid
 * as well as some fake entries so that applications using
 * passwd are lucky.
 *
 **********************************************************/
 
static NSS_STATUS build_pwd(const char *pwnam,
                            uid_t uid,
                            struct passwd *result,
                            char *buffer,
                            size_t buflen,
                            int *errnop)
{
    size_t pos = 0;
    int len = strlen(pwnam);

    if ( pos + len + 1 < buflen )
    {
       result->pw_name = buffer;
       strcpy(buffer, pwnam);
       SETPOS(pos,len+1);
    }
    else
    {
        *errnop = ERANGE;
        return NSS_STATUS_TRYAGAIN;
    }
    
    if ( pos + 2 < buflen )
    {
       result->pw_passwd = buffer+pos;
       strcpy(buffer+pos, "+");
       SETPOS(pos,2);
    }
    else
    {
        *errnop = ERANGE;
        return NSS_STATUS_TRYAGAIN;
    }

    result->pw_uid = uid;

    /* normally the primary group, on Linux and openSolaris
     * the promary group has mostly the same id as the
     * user so we put the uid here.
     * This may be wrong, but we will not allow a login
     * with this identity so that problems shall normally
     * not occurs.
     */
    result->pw_gid = uid;

    if ( pos + len + 1 < buflen )
    {
        result->pw_gecos = buffer+pos;
        strcpy(buffer+pos, pwnam);
        SETPOS(pos,len+1);
    }
    else
    {
        *errnop = ERANGE;
        return NSS_STATUS_TRYAGAIN;
    }

    if ( pos + 5 < buflen )
    {
        result->pw_dir = buffer+pos;
        strcpy(buffer+pos, "/tmp");
        SETPOS(pos,5);
    }
    else
    {
        *errnop = ERANGE;
        return NSS_STATUS_TRYAGAIN;
    }

    if ( pos + 10 < buflen )
    {
        result->pw_shell = buffer+pos;
        strcpy(buffer+pos, "/bin/false");
        SETPOS(pos,10);
    }
    else
    {
        *errnop = ERANGE;
        return NSS_STATUS_TRYAGAIN;
    }

    return NSS_STATUS_SUCCESS;
}

/**********************************************************
 *
 * build_grp()
 * Fill a group structur with the group name and it gid
 * as well as some fake entries so that applications using
 * passwd are lucky.
 *
 **********************************************************/
 
static NSS_STATUS build_grp(const char *grnam,
                            gid_t gid,
                            struct group *result,
                            char *buffer,
                            size_t buflen,
                            int *errnop)
{
    size_t pos = 0;
    int len = strlen(grnam);
    if ( pos + len + 1 < buflen )
    {
       result->gr_name = buffer;
       strcpy(buffer, grnam);
       SETPOS(pos,len+1);
    }
    else
    {
        *errnop = ERANGE;
        return NSS_STATUS_TRYAGAIN;
    }
    
    if ( pos + 2 < buflen )
    {
       result->gr_passwd = buffer+pos;
       strcpy(buffer+pos, "+");
       SETPOS(pos,2);
    }
    else
    {
        *errnop = ERANGE;
        return NSS_STATUS_TRYAGAIN;
    }

    result->gr_gid = gid;
    
    result->gr_mem = NULL;

    return NSS_STATUS_SUCCESS;

}

/**********************************************************
 *
 * _nss_rfs_getpwnam_r()
 *
 *
 **********************************************************/
 
NSS_STATUS _nss_rfs_getpwnam_r(const char *pwnam,
                               struct passwd *result,
                               char *buffer,
                               size_t buflen,
                               int *errnop)
{
    NSS_STATUS ret = NSS_STATUS_UNAVAIL;
    uid_t uid = 0;
    ret = query_server(GETPWNAM, (char*)pwnam, &uid, errnop);
    if ( *errnop == 0 && ret == NSS_STATUS_SUCCESS )
    {
        ret = build_pwd(pwnam, uid, result, buffer, buflen, errnop);
    }
    return ret;
}

/**********************************************************
 *
 * _nss_rfs_getpwuid_r()
 *
 *
 **********************************************************/
 
NSS_STATUS _nss_rfs_getpwuid_r(uid_t uid,
                               struct passwd *result,
                               char *buffer,
                               size_t buflen,
                               int *errnop)
{
    NSS_STATUS ret = NSS_STATUS_UNAVAIL;
    char pwnam[RFS_LOGIN_NAME_MAX+1];
    ret = query_server(GETPWUID, pwnam, &uid, errnop);
    if ( *errnop == 0 && ret == NSS_STATUS_SUCCESS )
    {
        ret = build_pwd(pwnam, uid, result, buffer, buflen, errnop);
    }
    return ret;
}

/**********************************************************
 *
 * _nss_rfs_getgrnam_r()
 *
 *
 **********************************************************/

NSS_STATUS _nss_rfs_getgrnam_r(const char *grnam,
                              struct group *result,
                              char *buffer,
                              size_t buflen,
                              int *errnop)
{
    NSS_STATUS ret = NSS_STATUS_UNAVAIL;
    gid_t gid = 0;
    ret = query_server(GETGRNAM, (char*)grnam, &gid, errnop);
    if ( *errnop == 0 && ret == NSS_STATUS_SUCCESS )
    {
        ret = build_grp(grnam, gid, result, buffer, buflen, errnop);
    }
    return ret;

}

/**********************************************************
 *
 * _nss_rfs_getgrgid_r()
 *
 *
 **********************************************************/
 
NSS_STATUS _nss_rfs_getgrgid_r(uid_t gid,
                               struct group *result,
                               char *buffer,
                               size_t buflen,
                               int *errnop)
{
    NSS_STATUS ret = NSS_STATUS_UNAVAIL;
    char grnam[RFS_LOGIN_NAME_MAX+1];
    ret = query_server(GETGRGID, grnam, &gid, errnop);
    if ( *errnop == 0 && ret == NSS_STATUS_SUCCESS )
    {
        ret = build_grp(grnam, gid, result, buffer, buflen, errnop);
    }
    return ret;

}

/* The following included files contain the initialization and
 * wrapper for call of the _nss_rfs*_r() functions which are
 * sufficients for Linux.
 */

#if defined FREEBSD
#include "rfs_nss_freebsd.c"
#endif
#if defined SOLARIS
#include "rfs_nss_solaris.c"
#endif
