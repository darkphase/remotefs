/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

/* rfs_nss_server.c
 *
 * communiocate with local rds clients and
 * libnss_rfs
 *
 */
 
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>
#include <pwd.h>
#include <grp.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <sys/stat.h>

#include "rfs_nss.h"
#include "dllist.h"

typedef struct rfs_idmap_s
{
    char *name;
    uid_t id;
} rfs_idmap_t;

static list_t *user_list = NULL;
static list_t *group_list = NULL;
static int     log = 0;
static int     connections = 1; /* number of rfs client */

/***************************************
 *
 * signal_handler()
 *
 * handle termination
 *
 */

static void signal_handler(int code)
{
    if (log ) printf("Signal %d received, terminate\n",code);
    unlink(SOCKNAME);
    exit(1);
}

/***************************************
 *
 * calculate_hash()
 *
 * calculate a hash value with a value between
 * 10000 and 19999
 *
 */

static int calculate_hash(const char *buf)
{
    int len = strlen(buf);
    size_t n;

    uint32_t s1 = 1;
    uint32_t s2 = 0;

    for (n = 0; n < len; n++)
    {
        s1 = (s1 + buf[n]) % 65521;
        s2 = (s2 + s1) % 65521;
    }
    return (((s2 << 16) | s1)%10000)+10000;
}

/***************************************
 *
 * open_socket()
 *
 * open the socket we will use for listening
 *
 */

static int open_socket(void)
{
    int sock = -1;
    if ( (sock = socket(PF_UNIX, SOCK_STREAM, 0)) == -1 )
    {
       if ( log ) perror("socket");
    }
    else
    {
        struct sockaddr_un sock_address;
        memset(&sock_address, 0, sizeof(struct sockaddr_un));
        strcpy(sock_address.sun_path, SOCKNAME );
        sock_address.sun_family = AF_UNIX;

        int ret = bind(sock, (struct sockaddr*)&sock_address, sizeof(struct sockaddr_un));
        if ( ret == -1 )
        {
            if ( log ) perror("bind");
            close(sock);
            sock = -1;
        }
        /* allow every one to connect to the socket */
        chmod(SOCKNAME, 0777);
    }

    return sock;
}

/***************************************
 *
 * process_message()
 *
 * get ans answer the messages from the resolver
 * library
 *
 */

static int process_message(int sock)
{
    cmd_t command;
    list_t *l = NULL;
    /* get message */
    int ret = recv(sock, &command, sizeof(command), 0);
    if ( ret == -1 )
    {
        return 0;
    }

    command.found = 0; /* preset to NOT FOUND */

    switch(command.cmd)
    {
        case CHECK_SERVER:
             return 1;
        break;
        case DEC_CONN:
            connections--;
            if ( log ) printf("Number of rfs clients: %d\n", connections);
            return 1;
        break;
        case INC_CONN:
            connections++;
            if ( log ) printf("Number of rfs clients: %d\n", connections);
            return 1;
        break;
        case GETGRNAM:
        case GETPWNAM:
            if ( log ) printf("Message received: %s(%s)\n",
                              command.cmd == GETGRNAM
                                  ? "getgrnam" : "getpwnam",
                              command.name);
            command.id = 0;
            if ( command.cmd == GETGRNAM )
                l = group_list;
            else
                l = user_list;

            /* search entry for given name */
            while ( (l = list_iterate(user_list, &l)) )
            {
               rfs_idmap_t *u = (rfs_idmap_t*)l->data;

               if ( strncmp( u->name, command.name, 8) == 0 )
               {
                   command.id = u->id;
                   command.found = 1;
                   break;
               }
            }

            if (command.found == 0 )
            {
                /* the name was noz found, calculate an id */
                int hashVal = calculate_hash(command.name);
                /* add entry */
                if ( command.cmd == GETGRNAM )
                    l = group_list;
                else
                    l = user_list;

                /* and insert this into our list */
                while (l)
                {
                    if ( ((rfs_idmap_t*)(l->data))->id > hashVal )
                    {
                        /* mo mame with this id, insert data */
                        rfs_idmap_t * map = calloc(sizeof(rfs_idmap_t),1);
                        if ( map == NULL )
                        {
                             if ( log ) perror("calloc");
                             return 0;
                        }

                        map->id = hashVal;
                        map->name = strdup(command.name);
                        if ( map->name == NULL )
                        {
                             if ( log ) perror("calloc");
                             return 0;
                        }

                        command.found = 1;
                        command.id = hashVal;
                        if ( list_insert(&l, map, NULL) == 0 )
                        {
                             if ( log ) perror("calloc");
                             return 0;
                        }
                        break;
                    }
                    else if ( ((rfs_idmap_t*)(l->data))->id == hashVal )
                    {
                       /* the id is allready used, increase (a new id) */
                       hashVal++;
                    }
                    l = l->next;
                }
            }
        break;
        case GETGRGID:
        case GETPWUID:
            if ( log ) printf("Message received: %s(%d)\n",
                              command.cmd == GETGRGID
                                  ? "getgrgid" : "getgrgid",
                              command.id);
            command.name[0] = '\0';
            if ( command.cmd == GETGRGID )
                l = group_list;
            else
                l = user_list;
            /* search only the corresponding entry */
            while ( (l = list_iterate(user_list, &l)) )
            {
               rfs_idmap_t *u = (rfs_idmap_t*)l->data;
               if ( u->id == command.id )
               {
                   strncpy(command.name, u->name, 8);
                   command.name[8] = 0;
                   command.found   = 1;
                   break;
               }
            }
        break;
    }

    if ( log ) printf("    Answer: name = %s, id = %d\n",
                      command.name, command.id);

    send(sock, &command, sizeof(command),0 );
    return 1; /* all was OK */
}

/***************************************
 *
 * main_loop()
 *
 * wait for message from the resolver library
 * and process them
 *
 */

static void main_loop(int sock)
{
    int ret;
    int accpt;
    struct sockaddr_un sock_address;
    socklen_t    sockLen = sizeof(sock_address);
    memset(&sock_address, 0, sizeof(sock_address));
    while(connections > 0)
    {
        if ( listen(sock, 1) >= 0 )
        {
           accpt = accept(sock, (struct sockaddr*)&sock_address, &sockLen);
           if ( accpt == -1 )
           {
               if ( log ) perror("accept");
               return;
           }
           else
           {
               ret = process_message(accpt);
               close(accpt);
               if ( ret == 0 )
               {
                  return;
               }
           }
        }
        else
        {
            if ( log ) perror("listen");
            return;
        }
    }
    if ( log ) printf("No more rfs clients, terminate\n");
}

/***************************************
 *
 * add_to_list()
 *
 * add the given data so that we have a
 * sorted list
 *
 */

static int add_to_list(list_t **root, rfs_idmap_t *data, uid_t id)
{
    int ret = 1;
    if ( *root == NULL )
    {
       ret = list_insert(root, data, NULL);
    }
    else
    {
         list_t *elem = NULL;
         while ( (elem = list_iterate(*root, &elem)) )
         {
            if( ((rfs_idmap_t*)(elem->data))->id > id)
            {
                 ret = list_insert(&elem, data, NULL);
                 break;
            }
            else if ( elem->next == NULL )
            {
                 ret =list_add(&elem, data, NULL);
                 break;
            }
         }
     }
     return ret;
}

/***************************************
 *
 * main()
 *
 *
 */

int main(int argc,char **argv)
{
    struct passwd *pwd;
    struct group  *grp;
    rfs_idmap_t   *user_map;
    rfs_idmap_t   *group_map;
    int            sock = -1;
    int            ret  = 1;
    int            daemonize = 1;
    int            opt;

    while ( (opt = getopt(argc, argv, "fl")) != -1 )
    {
        switch(opt)
        {
             case 'f': daemonize = 0; break;
             case 'l': log = 1; break;
        }
    }

    /* check first for eunning server */
    switch (control_rfs_nss(CHECK_SERVER))
    {
        case RFS_NSS_OK:
            /* a server is running, don't start */
            printf("An other server is allready running!\n");
            return 1;
        break;
        case RFS_NSS_SYS_ERROR:
        case RFS_NSS_NO_SERVER:
            /* no server detected ! */
            unlink(SOCKNAME);
        break;
    }
    
    /* collect the known user and groups */
    setpwent();
    while( (pwd = getpwent()) && ret )
    {
       user_map = (rfs_idmap_t*) calloc(sizeof(rfs_idmap_t),1);
       if ( user_map == NULL )
       {
            if ( log ) perror("calloc");
            exit(1);
       }
       user_map->name = strdup(pwd->pw_name);
       if ( user_map->name == NULL )
       {
            if ( log ) perror("strdup");
            exit(1);
       }
       user_map->id = pwd->pw_uid;
       ret = add_to_list(&user_list, user_map, (uid_t)pwd->pw_uid);
    }
    endpwent();

    if ( ret == 0 )
    {
       /* no enough memory ? we have a big problem and terminate */
       exit(1);
    }

    setgrent();
    while((grp = getgrent()))
    {
       group_map = (rfs_idmap_t*) calloc(sizeof(rfs_idmap_t),1);
       if ( group_map == NULL )
       {
            if ( log ) perror("calloc");
            exit(1);
       }
       group_map->name = strdup(grp->gr_name);
       if ( group_map->name == NULL )
       {
            if ( log ) perror("strdup");
            exit(1);
       }
       group_map->id   = grp->gr_gid;
       ret = add_to_list(&group_list, group_map, (uid_t)grp->gr_gid);
    }
    endgrent();

    if ( ret == 0 )
    {
       /* no enough memory ? we have a big problem and terminate */
       exit(1);
    }

    /* at this stage we have collected all id known for
     * this system.
     * We can now open our connection for our nss
     * library.
     */

     if ( (sock = open_socket()) < 0 )
     {
         unlink(SOCKNAME);
         exit(1);
     }

     signal(SIGTERM, signal_handler);
     signal(SIGINT,  signal_handler);
     signal(SIGQUIT, signal_handler);

     /* damonize */
     if (daemonize && fork() != 0)
     {
         return 0;
     }

     /* ceate pid file */
     FILE *fp = fopen(PIDFILE, "w");
     if ( fp )
     {
         fprintf(fp,"%d\n",getpid());
         fclose(fp);
     }

     main_loop(sock);

     close(sock);
     unlink(SOCKNAME);
     unlink(PIDFILE);

    return 0;
}
