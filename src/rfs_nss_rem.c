/* rfs_nss_rem_rfs_server.c
 *
 * send user and group name to the client rfs_nss_ask_rfs_server program
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pwd.h>
#include <grp.h>

#include "rfs_nss.h"

/**************************************************
 * process_message()
 *
 * A server has connected to the server, process
 * all message from client.
 *
 * int sock
 *
 * return 1 (OK)
 *
 *************************************************/

static int process_message(int sock)
{
    cmd_t          command;
    struct passwd *pwd;
    struct group  *grp;

    while( recv(sock, &command, sizeof(command), 0) == sizeof(command))
    {
        switch(htonl(command.cmd))
        {
            case SETPWENT:
                setpwent();
                command.found = htonl(1);
            break;
            case ENDPWENT:
                endpwent();
                command.found = htonl(1);
            break;
            case SETGRENT:
                setgrent();
                command.found = htonl(1);
            break;
            case ENDGRENT:
                endgrent();
                command.found = htonl(1);
            break;
            case GETPWENT:
                if ( (pwd = getpwent()) )
                {
                    strncpy(command.name, pwd->pw_name, RFS_LOGIN_NAME_MAX);
                    command.name[RFS_LOGIN_NAME_MAX] = '\0';
                    command.found = htonl(1);
                }
            break;
            case GETGRENT:
                if ( (grp = getgrent()) )
                {
                    strncpy(command.name, grp->gr_name, RFS_LOGIN_NAME_MAX);
                    command.name[RFS_LOGIN_NAME_MAX] = '\0';
                    command.found = htonl(1);
                }
            break;
            default: break;
        }
        send(sock, &command, sizeof(command), 0);
    }
    return 1;
}

/**************************************************
 * main_loop()
 *
 * open a socket for listening, wait for client
 * and if a client connect to us process all messages.
 *
 * char *listen_on NULL list on all inteface
 *                else liten on given address
 * int   port     port tp listen
 *
 * return 0 (Error), 1 (OK)
 *
 *************************************************/

static int main_loop(char *listen_on, int port)
{
    struct addrinfo *addr_info = NULL;
    struct addrinfo hints = { 0 };
    int sock = -1;
    struct timeval socket_timeout = { 10, 0 }; // TBD timeout from configuration
    int accpt;
#ifdef WITH_IPV6
    socklen_t sock_len = sizeof(struct sockaddr_in6);
    struct sockaddr_in6 sock_address;
#else
    socklen_t sock_len = sizeof(struct sockaddr_in);
    struct sockaddr_in sock_address;
#endif

    memset(&sock_address,0, sizeof(sock_address));
    
    if ( listen_on )
    {
        /* resolve IP for this name (may be a address */
        /* resolve name or address */
        hints.ai_family    = AF_UNSPEC;
        hints.ai_socktype  = SOCK_STREAM;
        hints.ai_flags     = AI_ADDRCONFIG;
        int result = getaddrinfo(listen_on, NULL, &hints, &addr_info);
        if (result != 0)
        {
             fprintf(stderr,"Can't resolve address: %s\n", gai_strerror(result));
             return 0;
        }

        if (addr_info->ai_family == AF_INET)
        {
            struct sockaddr_in *addr = (struct sockaddr_in *)addr_info->ai_addr;
            addr->sin_port = htons(port);
            sock = socket(AF_INET, SOCK_STREAM, 0);
            sock_len = sizeof(struct sockaddr_in);
        }
#ifdef WITH_IPV6
        else
        {
            struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr_info->ai_addr;
            addr6->sin6_port = htons(port);
            sock = socket(AF_INET6, SOCK_STREAM, 0);
            sock_len = sizeof(struct sockaddr_in6);
        }
#endif
        if ( sock != -1 )
        {
            if ( bind(sock, (struct sockaddr*)addr_info->ai_addr, sock_len) == -1 )
            {
                 close(sock);
                 return 0;
            }
        }
    }
    else
    {
#ifdef WITH_IPV6
        sock = socket(AF_INET6, SOCK_STREAM, 0);
        sock_len = sizeof(struct sockaddr_in6);
        sock_address.sin6_port = htons(port);
#else
        sock = socket(AF_INET, SOCK_STREAM, 0);
        sock_len = sizeof(struct sockaddr_in);
        sock_address.sin_port = htons(port);
#endif
        if ( sock != -1 )
        {
            if ( bind(sock, (struct sockaddr*)&sock_address, sock_len) == -1 )
            {
                 close(sock);
                 return 0;
            }
        }
    }

    /* listen */
    for(;;)
    {
        if ( listen(sock, 1) >= 0 )
        {
            accpt = accept(sock, (struct sockaddr*)&sock_address, &sock_len);
            if ( accpt == -1 )
            {
                return 0;
            }
            else
            {
                process_message(accpt);
                close(accpt);
            }
        }
    }
}

/**************************************************
 * syntax()
 *
 * print out how to call this program
 *
 * char *prog_name our name
 *
 * return nothing
 *
 *************************************************/

static void syntax(char *prog_name)
{
    fprintf(stderr,"Syntax: %s [-f] [-a listen_address] [-p port]\n",prog_name);
}


int main(int argc, char **argv)
{
    char *prog_name = strrchr(argv[0],'/');
    int   port = RFS_REM_PORT; // TBD from include file
    int   opt;
    int daemonize = 1;
    char *listen_on = NULL;

    if ( prog_name )
    {
        prog_name++;
    }
    else
    {
        prog_name = argv[0];
    }

    while( (opt = getopt(argc, argv, "a:p:f")) != -1 )
    {
        switch(opt)
        {
            case 'p': port = atoi(optarg); break;
            case 'a': listen_on = optarg; break;
            case 'f': daemonize = 0; break;
            default:  syntax(prog_name);return 1;
        }
    }

    if (daemonize && fork() != 0)
    {
        return 0;
    }

    main_loop(listen_on, port);

    return 0;
}
