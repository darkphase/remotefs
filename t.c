/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <stdio.h>
#include <pwd.h>
#include <grp.h>

int main(int argc, char **argv)
{
    struct passwd *pwd;
    struct group *grp;
    int error;
    uid_t uid = -1;
    gid_t gid = -1;
    char *name = "alice";
    if ( argc > 1 )
    {
       name = argv[1];
    }

    pwd = getpwnam(name);
    if ( pwd )
    {
        printf("Uid for user %s is %d\n",name, pwd->pw_uid);
        uid = pwd->pw_uid;
    }
    else
    {
        printf("Cant assign an uid for user %s\n",name);
    }

    pwd = getpwuid(uid);
    if (pwd)
    {
        printf("Uid %d -> %s\n",pwd->pw_uid, pwd->pw_name);
    }
    else
    {
       printf("User with uid %d not found\n",uid);
    }

    if ( argc > 2 )
    {
       name = argv[2];
    }
    else
    {
       name = "bob";
    }
    grp = getgrnam(name);
    if ( grp )
    {
        printf("Gid for group %s is %d\n",name, grp->gr_gid);
        gid = grp->gr_gid;
    }
    else
    {
        printf("Cant assign an gid for group %s\n",name);
    }

    grp = getgrgid(gid);
    if (grp)
    {
        printf("Gid %d -> %s\n",grp->gr_gid, grp->gr_name);
    }
    else
    {
       printf("Group with gid %d not found\n",uid);
    }

    return 0;
}
