/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <stdio.h>
#include <pwd.h>
#include <grp.h>

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "../src/rfs_nss.h"

int rfs_nss_send(cmd_t *cmd)
{
    /* connect with the rfs clients */
    int ret = RFS_NSS_OK;
    int sock;
    struct stat status;
    
    /* check if the socket exist */
    if ( (ret = stat(SOCKNAME, &status)) == -1 )
    {
        printf("Server socket not present\n");
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
       ret = send(sock, cmd, sizeof(cmd_t), 0);
       if ( ret == -1 )
       {
           perror("send");
           return RFS_NSS_SYS_ERROR;
       }
    }

    close(sock);
    return RFS_NSS_OK;
}

int main(int argc, char **argv)
{
    struct passwd *pwd;
    struct group *grp;
    uid_t uid = -1;
    gid_t gid = -1;
    char *name = "alice";
    char *server_name = "example.net";

    if ( argc > 1 )
    {
       name = argv[1];
    }

    printf("Check for user %s\n",name);
    pwd = getpwnam(name);
    if ( pwd )
    {
        printf("    Uid for user %s is %d\n",name, pwd->pw_uid);
        uid = pwd->pw_uid;
    }
    
    if (pwd == NULL )
    {
       cmd_t command = {0, };
       snprintf(command.name, sizeof(command.name), "%s@%s",name,server_name);
       printf("Check for user %s\n",command.name);
       command.cmd  = PUTPWNAM;
       rfs_nss_send(&command);
       pwd = getpwnam(command.name);
       if ( pwd )
       {
           printf("    Uid for user %s is %d\n",pwd->pw_name, pwd->pw_uid);
           uid = pwd->pw_uid;
       }
    }

    if ( uid != -1 )
    {
       pwd = getpwuid(uid);
       if (pwd)
       {
           printf("    Uid %d -> %s\n",pwd->pw_uid, pwd->pw_name);
       }
    }

    if ( argc > 2 )
    {
       name = argv[2];
    }
    else
    {
       name = "bob";
    }

    printf("Check for group %s\n",name);
    grp = getgrnam(name);
    if ( grp )
    {
        printf("Gid for group %s is %d\n",name, grp->gr_gid);
        gid = grp->gr_gid;
    }

    if (grp == NULL )
    {
       cmd_t command = {0, };
       snprintf(command.name, sizeof(command.name), "%s@%s",name,server_name);
       printf("Check for group %s\n",command.name);
       command.cmd  = PUTGRNAM;
       rfs_nss_send(&command);
       grp = getgrnam(command.name);
       if ( grp )
       {
           printf("    Gid for group %s is %d\n",grp->gr_name, grp->gr_gid);
           gid = grp->gr_gid;
       }
    }

    if (gid != -1 )
    {
        grp = getgrgid(gid);
        printf("    Gid %d -> %s\n",grp->gr_gid, grp->gr_name);
    }

    return 0;
}
