#include <stdio.h>
#include <unistd.h>
#include <string.h>
 #include <dlfcn.h>

#include "rfs_nss.h"

static void syntax(char *prog_name)
{
    fprintf(stderr,
            "Syntax: %s -u user | -g group [-h host]\n",
            prog_name);
}

int main(int argc, char **argv)
{
    int   ret;
    char *user = NULL;
    char *group = NULL;
    char *host = NULL;
    char *prog_name = strrchr(argv[0], '/');
    void *dl_hdl = NULL;
    uid_t (*putpwnam)(char*, char*);
    gid_t (*putgrnam)(char*, char*);

    if ( prog_name != NULL )
    {
       prog_name++;
    }
    else
    {
       prog_name = argv[0];
    }

    if ( (dl_hdl=dlopen(LIBRFS_NSS, RTLD_LAZY)) == NULL )
    {
        fprintf(stderr,
                "%s: can't open library %s\n",
                prog_name,
                LIBRFS_NSS);
        return 1;
    }

    if ( (putpwnam = (uid_t (*)(char*, char*))dlsym(dl_hdl,"rfs_putpwnam")) == NULL )
    {
        dlerror();
        return 1;
    }

    if ( (putgrnam = (gid_t (*)(char*, char*))dlsym(dl_hdl, "rfs_putgrnam")) == NULL )
    {
        dlerror();
        return 1;
    }

    while ( (ret=getopt(argc, argv,"u:g:h:")) != -1)
    {
        switch(ret)
        {
           case 'u': user  = optarg; break;
           case 'g': group = optarg; break;
           case 'h': host  = optarg; break;
           default:
              syntax(prog_name);
              return 1;
        }
    }
    
    if ( !user && !group )
    {
        syntax(prog_name);
        return 1;
    }
    
    if ( user )
    {
       uid_t uid = putpwnam(user, host);
       if ( uid == 0 )
       {
           fprintf(stderr,"%s: user %s not added\n",prog_name, user);
           return 1;
       }
    }

    if ( group )
    {
       uid_t uid = (uid_t)-1;
       uid = putgrnam(group, host);
       if ( uid == 0 )
       {
           fprintf(stderr,"%s: group %s not added\n",prog_name, user);
           return 1;
       }
    }
    return 0;
}
