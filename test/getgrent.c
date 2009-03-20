#include <stdio.h>
#include <pwd.h>
#include <grp.h>

int main(int argc, char **argv)
{
    struct group  *grp;
    int set_end[4];
    char *s;
    set_end[0] = 1;
    set_end[1] = 0;
    set_end[2] = 0;
    set_end[3] = 0;
    if ( argc > 1 )
    {
        int i;
        s = argv[1];
        for (i=0; *s && i < 4; s++,i++)
        {
            if ( *s == '0' ) set_end[i] = 0;
        }
    }


    if ( set_end[0] ) setgrent();
    while( (grp = getgrent()))
    {
        if(grp->gr_gid > 9999 && grp->gr_gid <21000 )
        {
            printf("%-20s %d\n", grp->gr_name, grp->gr_gid);
        }
    }
    if ( set_end[1] ) endgrent();

    if ( set_end[2] ) endgrent();
    while( (grp = getgrent()))
    {
        if(grp->gr_gid > 9999 && grp->gr_gid <21000 )
        {
            printf("%-20s %d\n", grp->gr_name, grp->gr_gid);
        }
    }
    if ( set_end[3] ) endgrent();
    return 0;
}
