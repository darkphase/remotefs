/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <stdio.h>
#include <pwd.h>
#include <grp.h>

#include "../src/rfs_nss.h"


int main(int argc, char **argv)
{
    struct passwd *pwd;
    struct group *grp;
    uid_t uid = (uid_t)-1;
    gid_t gid = (uid_t)-1;
    char *name = "alice";
    char *server_name = "example.net";
    char  log_host_name[RFS_LOGIN_NAME_MAX];
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
       snprintf(log_host_name, sizeof(log_host_name), "%s@%s", name, server_name);
       printf("Check for user %s\n",log_host_name);
       control_rfs_nss(PUTPWNAM, name, server_name, &uid);
       if ( uid )
       {
           printf("    Uid for user %s is %d (putpwnam)\n",log_host_name, uid);
       }
       uid = (uid_t)-1;
       pwd = getpwnam(log_host_name);
       if ( pwd )
       {
           printf("    Uid for user %s is %d (getpwnam)\n",pwd->pw_name, pwd->pw_uid);
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
       snprintf(log_host_name, sizeof(log_host_name), "%s@%s",name,server_name);
       printf("Check for group %s\n",log_host_name);
       control_rfs_nss(PUTGRNAM, name, server_name, (uid_t*)&gid);
       grp = getgrnam(log_host_name);
       if ( grp )
       {
           printf("    Gid for group %s is %d\n",grp->gr_name, grp->gr_gid);
           gid = grp->gr_gid;
       }
    }

    if (gid != -1 )
    {
        grp = getgrgid(gid);
        if ( grp )
        {
            printf("    Gid %d -> %s\n",grp->gr_gid, grp->gr_name);
        }
    }

    return 0;
}
