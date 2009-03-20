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
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "rfs_nss.h"

/************************************************************
 * control_rfs_nss
 *
 * Send a command to rfs_nss.
 *
 * int   cmd   - INC_CONN, DEC_CONN
 *               tell rfs_nss that a cleint start/stop,
 *             - CHECK_SERVER
 *               check id server is running
 *             - PUTPWNAM, PUTGRNAM
 *               tell rfs_nss to store the user/group name
 *               optionnaly for the given host
 * char *name  user/group name
 * char *host  host name or IP
 *
 * name and host are madatory for PUTPWNAM and PUTGRNAM and
 * not used for the other commands.
 *
 * Return RFS_NSS_OKR, RFS_NSS_NO_SERVERR oer RFS_NSS_SYS_ERROR
 *
 * RFS_NSS_SYS_ERROR is return on parameter error or system call
 * errors.
 *
 ***********************************************************/
 
int control_rfs_nss(const int cmd, const char *name, const char *host, uid_t *id)
{
    /* connect with the rfs clients */
    int ret = RFS_NSS_OK;
    int sock;
    cmd_t command;
    struct stat status;

    /* check for parameters */
    if ( cmd == PUTPWNAM || cmd == PUTGRNAM )
    {
        if ( name == NULL )
        {
            return RFS_NSS_SYS_ERROR;
        }
    }
    else if ( !(cmd == CHECK_SERVER || cmd == INC_CONN || cmd == DEC_CONN) )
    {
        return RFS_NSS_SYS_ERROR;
    }

    /* check if the socket exist */
    if ( (ret = stat(SOCKNAME, &status)) == -1 )
    {
        return RFS_NSS_NO_SERVER;
    }

    sock = socket(PF_UNIX, SOCK_STREAM, 0);
    if ( sock == -1 )
    {
         perror("connect");
         return RFS_NSS_SYS_ERROR;
    }

    struct sockaddr_un sock_address;
    memset(&sock_address, 0, sizeof(struct sockaddr_un));
    strcpy(sock_address.sun_path, SOCKNAME);
    sock_address.sun_family = AF_UNIX;

    ret = connect(sock, (struct sockaddr*)&sock_address, sizeof(struct sockaddr_un));
    if ( ret == -1 )
    {
         perror("connect");
         close(sock);
         return RFS_NSS_NO_SERVER;
    }
    else
    {
        command.cmd = cmd;
        if ( cmd == PUTPWNAM || cmd == PUTGRNAM )
        {
            if ( host )
            {
                snprintf(command.name, sizeof(command.name),"%s@%s",name,host);
            }
            else
            {
                snprintf(command.name, sizeof(command.name),"%s",name);
            }
        }

        if ( cmd == DEC_CONN || cmd == INC_CONN )
        {
            if ( host )
            {
                strncpy(command.name, host,sizeof(command.name));
            }
            if ( id )
            {
               command.caller_id = *id;
            }
            else
            {
               command.caller_id = 0;
            }
        }

        ret = send(sock, &command, sizeof(command), 0);
        if ( ret == -1 )
        {
            perror("send");
            close(sock);
            return RFS_NSS_SYS_ERROR;
        }
    }

    /* get the uid/gid for the given name */
    if ( cmd == PUTPWNAM || cmd == PUTGRNAM )
    {
       recv(sock, &command, sizeof(command), 0);
       *id = command.id;
    }

    close(sock);
    return RFS_NSS_OK;
}

uid_t rfs_putpwnam(char *user, char *host)
{
    uid_t uid = 0;
    control_rfs_nss(PUTPWNAM, user, host, &uid);
    return uid;
}

gid_t rfs_putgrnam(char *group, char *host)
{
    gid_t gid = 0;
    control_rfs_nss(PUTGRNAM, group, host, (uid_t*)&gid);
    return gid;
}
