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
#include <pwd.h>
#include <grp.h>

#include "rfs_nss.h"

/**************************************************
 * connect_to_host()
 *
 * Establish a connection to the given server and
 * resolve the host name so we can use it for build
 * of the user and group name
 * .
 * char   *rhost   The remote host to contact (ip or
                   name)
 * int     port    and the port to use
 *
 * char   *host    The host name will be placed here
 * size_t  hlen    size of the host array
 *
 * return the socket handle or -1
 *
 *************************************************/

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
    if ( *host == '\0' )
    getnameinfo((struct sockaddr *)addr_info->ai_addr,
                 addr_info->ai_addrlen, host, hlen, NULL, 0, 0);

    freeaddrinfo(addr_info);

    return sock;
}


/**************************************************
 * do_command()
 *
 * Send a query to the server and get it answer
 * .
 * cmd_t *command  buffer for communication
 * int    sock     our communication handle
 *
 * return 1 if OK else 0
 *
 *************************************************/

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

/**************************************************
 * ask_for_name()
 *
 * Call setXent(), getXent(),... endXent() on the
 * server and put the answer to rfs_nss
 * .
 * int   sock        our communication handle
 * char *type        "user" or "group"
 * char *host_name   name of remote host as known
 *                   on the client (this), may be
 *                   NULL
 *
 * return 1 if OK else 0
 *
 *************************************************/

int ask_for_name(int sock, char *type, char *host_name, int put_all)
{
    cmd_t command;
    int is_user = 1;
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
        is_user = 0;
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
             if (!put_all)
             {
                 /* if the user or group is known don't add */
                 if ( ( is_user && getpwnam(command.name) )
                      ||
                      ( !is_user && getgrnam(command.name) )
                    )
                 {
                     memset(&command, 0, sizeof(command));
                     command.cmd     = getcmd;
                     continue;
                 }
            }

             if ( strchr(command.name, '@') == NULL )
             {
                 /* we ignore possible errors here */
                 if ( is_user )
                 {
                     rfs_putpwnam(command.name, host_name);
                 }
                 else
                 {
                     rfs_putgrnam(command.name, host_name);
                 }
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

/**************************************************
 * syntax()
 *
 * print thze usage information
 * server and put the answer to rfs_nss
 * .
 * char *prog_name   the name of this program
 *
 * return nothings
 *
 *************************************************/

static void syntax(char *prog_name)
{
    fprintf(stderr,"Syntax: %s -h|H host_name_or_ip [-n host_name] [-p port] [-a]\n",prog_name);
    fprintf(stderr,"       - If you pass -H instead of -h, the name will be\n");
    fprintf(stderr,"         appended with @host_name_or_ip.\n");
    fprintf(stderr,"       - If you use the -h option and want to assign a\n");
    fprintf(stderr,"         host name to the server you can use the -n option,\n");
    fprintf(stderr,"       - With the -a option @host_name_or_ip is added\n");
    fprintf(stderr,"         to all even if there are locally known,\n");
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
    int  put_all = 0;

    if ( prog_name )
    {
        prog_name++;
    }
    else
    {
        prog_name = argv[0];
    }

    *host = '\0';
    
    while( (opt = getopt(argc, argv, "h:H:p:an:")) != -1 )
    {
        switch(opt)
        {
            case 'H': add_host = 1;
            case 'h': rhost = optarg; break;
            case 'p': port = atoi(optarg); break;
            case 'a': put_all = 1; break;
	    case 'n': add_host = 1; strncpy(host, optarg, NI_MAXHOST); break;
            default: syntax(prog_name);return 1;
        }
    }

    if ( rhost == NULL )
    {
        syntax(prog_name);
        return 1;
    }

    if ( add_host == 0 )
    {
        put_all = 0;
    }

    if ( (sock = connect_to_host(rhost, port, host, NI_MAXHOST )) != -1 )
    {
        if ( *host )
        {
            rhost = host;
        }
        /* ask for all user name and put them to our db server */
        ask_for_name(sock, "user", add_host?rhost:NULL, put_all);

        /* ask for all group name  put them to our db server */
        ask_for_name(sock, "group", add_host?rhost:NULL, put_all);

        close(sock);
    }
    else
    {
        fprintf(stderr, "%s: can't connect to %s\n", prog_name, rhost ? rhost : "server" );
        return 1;
    }

    return 0;
}
