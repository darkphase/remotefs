#include <stdio.h>
#include <pwd.h>
#include <grp.h>

int main(int argc, char **argv)
{
    struct passwd *pwd;
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

    if ( set_end[0]) setpwent();
    while( (pwd = getpwent()))
    {
        if ( *pwd->pw_passwd == '+' )
            printf("%-20s %d\n", pwd->pw_name, pwd->pw_uid);
    }
    if ( set_end[1]) endpwent();

    if ( set_end[2]) setpwent();
    while( (pwd = getpwent()))
    {
        if ( *pwd->pw_passwd == '+' )
            printf("%-20s %d\n", pwd->pw_name, pwd->pw_uid);
    }
    if ( set_end[3]) endpwent();
    return 0;
}
