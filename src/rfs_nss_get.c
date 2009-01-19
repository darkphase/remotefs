#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#if defined FREEBSD
#include <netinet/in.h>
#endif

#include "rfs_nss.h"


int connect_to_host(char *rhost, int port, char *host, size_t hlen)
{
    struct addrinfo *addr_info = NULL;
    struct addrinfo hints = { 0 };
    int sock = -1;
    struct timeval socket_timeout = { 10, 0 }; // TBD timeout from configuration

    /* resolve name or address */
    hints.ai_family    = AF_UNSPEC;
    hints.ai_socktype  = SOCK_STREAM; 
    hints.ai_flags     = AI_ADDRCONFIG | AI_CANONNAME;
    
    int result = getaddrinfo(rhost, NULL, &hints, &addr_info);
    if (result != 0)
    {
         fprintf(stderr,"Can't resolve address: %s\n", gai_strerror(result));
         return -1;
    }

    if (addr_info->ai_family == AF_INET)
    {
        struct sockaddr_in *addr = (struct sockaddr_in *)addr_info->ai_addr;
        addr->sin_port = htons(port);
        sock = socket(AF_INET, SOCK_STREAM, 0);
    }
#ifdef WITH_IPV6
    else
    {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr_info->ai_addr;
        addr6->sin6_port = htons(port);
        sock = socket(AF_INET6, SOCK_STREAM, 0);
    }
#endif

    if (sock == -1)
    {
        freeaddrinfo(addr_info);
        return -1;
    }

    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &socket_timeout, sizeof(socket_timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &socket_timeout, sizeof(socket_timeout));

    if (connect(sock, (struct sockaddr *)addr_info->ai_addr, addr_info->ai_addrlen) == -1)
    {
        perror("connect");
        freeaddrinfo(addr_info);
        return -1;
    }

    /* if we have passed an IP try to get the host name */
    getnameinfo((struct sockaddr *)addr_info->ai_addr,
                 addr_info->ai_addrlen, host, hlen, NULL, 0, 0);

    freeaddrinfo(addr_info);

    return sock;
}

int do_command(cmd_t *command, int sock)
{
    int ret;
    command->cmd = htonl(command->cmd);
    command->found = htonl(command->found);
    ret = send(sock, command, sizeof(cmd_t), 0);
    if ( ret ==  sizeof(cmd_t) )
    {
        ret = recv(sock, command, sizeof(cmd_t), 0);
        command->cmd = htonl(command->cmd);
        command->found = htonl(command->found);
    }

    if ( ret != sizeof(cmd_t) )
    {
       command->found = 0;
    }

    return command->found;
}

int ask_for_name(int sock, char *type, char *host_name)
{
    cmd_t command;

    /* set commands according to wanted entry type */
    int setcmd = SETPWENT;
    int getcmd = GETPWENT;
    int endcmd = ENDPWENT;
    int putcmd = PUTPWNAM;

    memset(&command, 0, sizeof(command));
    if ( strcmp(type,"group") == 0)
    {
        setcmd = SETGRENT;
        getcmd = GETGRENT;
        endcmd = ENDGRENT;
        putcmd = PUTGRNAM;
    }

     command.cmd     = setcmd;
     command.name[0] = '\0';
     command.id      = 0;
     command.found   = 0;

     if ( do_command(&command, sock) == 0 )
     {
         return 0;
     }

     command.cmd     = getcmd;
     command.name[0] = '\0';
     command.id      = 0;
     command.found   = 0;

     while(do_command(&command, sock) )
     {
         if ( command.found )
         {
             if ( strchr(command.name, '@') == NULL )
             {
             
                 /* we ignore possible errors here */
                 if ( putcmd == PUTPWNAM )
                     rfs_putpwnam(command.name, host_name);
                 else
                     rfs_putgrnam(command.name, host_name);
             }
         }
         memset(&command, 0, sizeof(command));
         command.cmd     = getcmd;
     }

     memset(&command, 0, sizeof(command));
     command.cmd     = endcmd;

     if ( do_command(&command, sock) == 0 )
     {
         return 0;
     }

     return 1;
}

static void syntax(char *prog_name)
{
    fprintf(stderr,"Syntax: %s -h|H host_name_or_ip [-p port]\n",prog_name);
    fprintf(stderr,"       If you pass -H instead of -h, the name will be\n");
    fprintf(stderr,"       appended with @host_name_or_ip\n");
}

int main(int argc, char **argv)
{
    char *rhost = NULL; /* address or name of remotefs server */
    char *prog_name = strrchr(argv[0],'/');
    int   port = RFS_REM_PORT;
    int   opt;
    int   sock = -1;
    int   add_host = 0;
    char  host[NI_MAXHOST];

    if ( prog_name )
    {
        prog_name++;
    }
    else
    {
        prog_name = argv[0];
    }

    while( (opt = getopt(argc, argv, "h:H:p:")) != -1 )
    {
        switch(opt)
        {
            case 'H': add_host = 1;
            case 'h': rhost = optarg; break;
            case 'p': port = atoi(optarg); break;
            default: syntax(prog_name);return 1;
        }
    }

    if ( rhost == NULL )
    {
        syntax(prog_name);
        return 1;
    }

    *host = '\0';
    if ( (sock = connect_to_host(rhost, port, host, NI_MAXHOST )) != -1 )
    {
        if ( *host )
        {
            rhost = host;
        }
        /* ask for all user name and put them to our db server */
        ask_for_name(sock, "user", add_host?rhost:NULL);

        /* ask for all group name  put them to our db server */
        ask_for_name(sock, "group", add_host?rhost:NULL);

        close(sock);
    }
    else
    {
        fprintf(stderr, "%s: can't connect to %s\n", prog_name, rhost ? rhost : "server" );
        return 1;
    }

    return 0;
}
