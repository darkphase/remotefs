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

int control_rfs_nss(int cmd)
{
    /* connect with the rfs clients */
    int ret = RFS_NSS_OK;
    int sock;
    cmd_t command;
    struct stat status;
    
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
       ret = send(sock, &command, sizeof(command), 0);
       if ( ret == -1 )
       {
           perror("send");
           return RFS_NSS_SYS_ERROR;
       }
    }

    close(sock);
    return RFS_NSS_OK;
}
