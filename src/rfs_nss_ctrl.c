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
#include <sys/stat.h>

#include "rfs_nss.h"
#include "rfs_getnames.h"

int main(int argc, char **argv)
{
    int ret = RFS_NSS_OK;
    int command = -1;
    char *prog_name = strrchr(argv[0], '/');
    int rfs_send_name = 0;
    char *host = NULL;

    if ( prog_name )
    {
        prog_name++;
    }
    else
    {
        prog_name = argv[0];
    }

    argc--;
    argv++;

    while ( argc > 0 )
    {
        if ( strcmp(argv[0], "stop") == 0 )
        {
            command = DEC_CONN;
        }
        else if ( strcmp(argv[0], "start") == 0 )
        {
            command = INC_CONN;
        }
        else if ( strcmp(argv[0], "check") == 0 )
        {
            command = CHECK_SERVER;
        }
        else if ( strcmp(argv[0], "-r") == 0 )
        {
            rfs_send_name = 1;
        }
        else if ( strcmp(argv[0], "-h") == 0 )
        {
            argv++;
            argc--;
            host = argv[0];
            if (host == NULL)
            {
               command = -1;
               break;
            }
        }
        else
        {
            command = -1;
            break;
        }
        argc--;
        argv++;
    }

    if ( command == -1 )
    {
        printf("Syntax: %s [-r] start [ip-or-host] |stop [ip-or-host] |check\n", prog_name);
        return 0;
    }

    if ( ret == 0 )
    {
        ret = control_rfs_nss(command, NULL, NULL, NULL);
    }

    switch(ret)
    {
        case RFS_NSS_OK:
            if ( command == INC_CONN )
            {
               printf("new client instance added\n");
               if ( host )
               {
                  get_all_names(host);
               }
            }
            else if ( command == DEC_CONN )
            {
               printf("new client instance removed\n");
            }
        break;
        case RFS_NSS_SYS_ERROR:
            printf("System error\n");
        break;
        case RFS_NSS_NO_SERVER:
             printf("Server not running\n");
             if ( command == INC_CONN )
             {
                 printf("Start rfs_nss\n");
                 if ( rfs_send_name )
                     ret = system("rfs_nss");
                 else
                     ret = system("rfs_nss");
                if ( ret == 0 && host )
                {
                    get_all_names(host);
                }
             }
        break;
    }
    return ret;
}
