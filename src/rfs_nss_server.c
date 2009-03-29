/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

/* rfs_nss_server.c
 *
 * communicate with local rfs clients and
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
#include <grp.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <netdb.h>
#if defined FREEBSD
#include <netinet/in.h>
#endif
#if defined QNX
#include <arpa/inet.h>
#endif

#include "rfs_nss.h"
#include "dllist.h"
#include "rfs_getnames.h"

typedef struct user_host_s
{
   char *host;
   int   hostid;
   int   count;
   list_t *users; /* data contain the uid instead is a pointer */
} user_host_t;


typedef struct rfs_idmap_s
{
    char *name;
    uid_t id;
    int   sys;
    int   hostid;
} rfs_idmap_t;


static list_t *user_list   = NULL;
static list_t *group_list  = NULL;
static list_t *hosts       = NULL;
static int     log         = 0;
static int     connections = 1; /* number of rfs client */

/**********************************************************
 *
 * signal_handler()
 *
 * handle termination
 *
 ***********************************************************/

static void signal_handler(int code)
{
    if (log ) printf("Signal %d received, terminate\n",code);
    unlink(SOCKNAME);
    exit(1);
}

/**********************************************************
 *
 * calculate_hash()
 *
 * calculate a hash value with a value between
 * 10000 and 19999
 *
 ***********************************************************/

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

/**********************************************************
 *
 * open_socket()
 *
 * open the socket we will use for listening
 *
 * Return socket handle or -1
 *
 ***********************************************************/

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

/**********************************************************
 *
 * search_name()
 *
 * get ans answer the messages from the resolver
 * library
 *
 * int sock the socked handle for comminication
 *
 * return 0 on error, 1 on succeess
 *
 ***********************************************************/

static inline list_t *search_name(cmd_t *command, list_t *list)
{
    while ( list )
    {
       rfs_idmap_t *idmap = (rfs_idmap_t*)list->data;

       if ( strncmp( idmap->name, command->name, RFS_LOGIN_NAME_MAX) == 0 )
       {
           command->id    = idmap->id;
           command->found = 1;
           break;
       }
       list = list->next;
    }
    return list;

}

/**********************************************************
 *
 * search_id()
 *
 * get ans answer the messages from the resolver
 * library
 *
 * int sock the socked handle for comminication
 *
 * return 0 on error, 1 on succeess
 *
 ***********************************************************/

static inline list_t *search_id(cmd_t *command, list_t *list)
{
    while ( list )
    {
       rfs_idmap_t *idmap = (rfs_idmap_t*)list->data;
       if ( ! idmap->sys && idmap->id == command->id )
       {
           strncpy(command->name, idmap->name, RFS_LOGIN_NAME_MAX);
           command->name[RFS_LOGIN_NAME_MAX] = 0;
           command->found   = 1;
           break;
       }
       list = list->next;
    }
    return list;
}

/**********************************************************
 *
 * add_user
 *
 * int   id   zid for the concerned user
 *
 *********************************************************/

static int add_user(list_t **users, int id)
{
    list_t *user = *users;
    list_t *new  = NULL;

    /* look if allready present */
    while(user)
    {
        if ( id == *((int*)(user->data)) )
        {
            break;
        }
        user = user->next;
    }

    /* there was not entry, build one */
    if ( user == NULL )
    {
        list_insert(users, NULL, &new);
    }

    if ( new )
    {
         new->data = (void*) malloc(sizeof(int));
         *((int*)(new->data)) = id;
    }
    else
    {
        return 0; /* new not allocated ! */
    }

    return 1;
}

/**********************************************************
 *
 * get_hostid
 *
 * char *name host name or user/group name (user@host)
 *
 *********************************************************/

static int get_hostid(char *name)
{
    list_t *host = hosts;
    char *nm;

    if ( name == NULL)
    {
        return 0;
    }

    /* get host name */
    if ( (nm = strchr(name, '@')) )
    {
       nm++;
    }
    else
    {
       nm = name;
    }

    /* search for the host id */
    while ( host )
    {
        if ( strcmp(nm, ((user_host_t*)(host->data))->host) == 0 )
        {
            return ((user_host_t*)(host->data))->hostid;
        }
        host = host->next;
    }
    return 0;
}

/**********************************************************
 *
 * add_host
 *
 * char *name host name
 * int   id   zid for the concerned user
 *
 *********************************************************/

static int add_host(char *name, int id)
{
    list_t *host = hosts;
    list_t *new  = NULL;
    int     hostid = 1;
    char *host_name;

    /* if allready provided increase only count */
    while(host)
    {
        host_name = ((user_host_t*)(host->data))->host;
        if ( strcmp(name, host_name) == 0 )
        {
            ((user_host_t*)(host->data))->count++;
            break;
        }
        host = host->next;
    }

    /* there was not entry, build one */
    if ( hosts == NULL )
    {
        list_insert(&hosts, NULL, &new);
    }
    else if ( host == NULL )
    {
        /* no entry, add one and set a unique hostid */
        host = hosts;
        while(host)
        {
            if ( ((user_host_t*)host->data)->hostid > hostid )
            {
                list_insert(&host, NULL, &new);
                break;
            }
            else if ( ((user_host_t*)host->data)->hostid == hostid )
            {
               hostid++;
            }
            else if ( host->next == NULL )
            {
               /* the end of list will be reached, append */
               list_append(&host->next, NULL, &new);
               break;
            }
            host = host->next;
        } 
    }

    if ( new )
    {
        new->data = (void*)calloc(sizeof(user_host_t),1);
        if ( new->data == NULL )
        {
            return 0;
        }
        ((user_host_t*)new->data)->hostid = hostid;
        ((user_host_t*)new->data)->host   = strdup(name);
        ((user_host_t*)new->data)->count  = 1;
        host = new;
    }
    else
    {
       return 0; /* error */
    }

    add_user(&(((user_host_t*)new->data)->users), id);

    return 1;
}


/**********************************************************
 *
 * user_is_concerned()
 *
 * check if the user which call this (user has mounted
 * resource for the  host referenced by mak->hostid
 *
 *return 1 if concerned else 0
 *
 **********************************************************/

static int user_is_concerned(rfs_idmap_t *map, int uid)
{
    list_t *host = hosts;
    list_t *user = NULL;

    while ( host )
    {
        if ( map->hostid == ((user_host_t*)(host->data))->hostid )
        {
            user = ((user_host_t*)(host->data))->users;
            while ( user )
            {
               if ( *((int*)(user->data)) == uid )
               {
                   return 1;
               }
               user = user->next;
            }
        }
        host = host->next;
    }
    return 0;
}

/**********************************************************
 *
 * remove_host()
 *
 * char *name host name for which entries are to be removed
 *
 *
 **********************************************************/

static int remove_host(char *name, int id)
{
    list_t *host   = hosts;
    list_t *user   = NULL;
    user_host_t *h = NULL;
    int     hostid = 1;

    /* find host */
    while ( host )
    {
        char *host_name = ((user_host_t*)(host->data))->host;
        if ( strcmp(name, host_name) == 0 )
        {
            break;
        }
        host = host->next;
    }

    if ( host )
    {
        int count = --((user_host_t*)(host->data))->count;
        if ( count <= 0 )
        {
           hostid = ((user_host_t*)(host->data))->hostid;

           /* all user and groups are to be removed */
           list_t *list = user_list;
           list_t *next = NULL;

           while(list)
           {
              next = list->next;
              if ( ((rfs_idmap_t*)(list->data))->hostid == hostid )
              {
                  if ( ((rfs_idmap_t*)(list->data))->name )
                  {
                      free(((rfs_idmap_t*)(list->data))->name);
                      free(list->data);
                      list_remove(&user_list, list);
                  }
              }
              list = next;
           }

           list = group_list;
           next = NULL;
           while(list)
           {
              next = list->next;
              if ( ((rfs_idmap_t*)(list->data))->hostid == hostid )
              {
                  if ( ((rfs_idmap_t*)(list->data))->name )
                  {
                      free(((rfs_idmap_t*)(list->data))->name);
                      free(list->data);
                      list_remove(&group_list, list);
                  }
              }
              list = next;
           }

           h = (user_host_t*)host->data;

           /* finaly remove the host istself */
           free(h->host);

           /* remove users member */
           free(h->users->data);
           list_remove(&h->users,h->users);

           free(h);
           list_remove(&hosts, host);
        }
        else
        {
           /* remove the user entry for id */
           user = ((user_host_t*)(host)->data)->users;
           while(user)
           {
              if ( *((int*)(user->data)) == id )
              {
                 free(user->data);
                 list_remove( & ((user_host_t*)host)->users, user);
              }
              else
              {
                 user = user->next;
              }
           }
        }
    }

    return 1;
}

/**********************************************************
 *
 * insert_new_idmap()
 *
 * list_t **root   pointer to the user or group list
 * cmd_t  *command the name member shall contain the user
 *                 or group name
 *
 * If all is OK command.id contain the assigned id and
 * command.found is to be set to 1
 *
 * return 1 if all is ok else 0 (value of command.found)
 * 
 ************************************************************/

static int insert_new_idmap(list_t **root, cmd_t *command, int hostid)
{
    /* the name was not found, calculate an id */
    int hashVal = 0;
    list_t *list = *root;

    command->found = 0; /* preset to not inserted */
    /* calculate an id */
    hashVal = calculate_hash(command->name);
    
    /* and insert this into our list */
    while (list)
    {
        if ( ((rfs_idmap_t*)(list->data))->id > hashVal )
        {
            /* no mame with this id, insert data */
            rfs_idmap_t * map = calloc(sizeof(rfs_idmap_t),1);
            if ( map == NULL )
            {
                 if ( log ) perror("calloc");
                 break;
            }

            map->id = hashVal;
            map->name = strdup(command->name);
            map->hostid = hostid;
            if ( map->name == NULL )
            {
                 if ( log ) perror("calloc");
                 break;
            }

            command->found = 1;
            command->id    = hashVal;
            if ( list_insert(&list, map, NULL) == 0 )
            {
                 if ( log ) perror("calloc");
                 break;
            }
            break;
        }
        else if ( ((rfs_idmap_t*)(list->data))->id == hashVal )
        {
           /* the id is allready used, increase (a new id) */
           hashVal++;
        }
        list = list->next;
    }
    return command->found;
}

/**********************************************************
 *
 * get_owner_entry()
 *
 * search the list element for the user/group of the
 * calling process
 *
 * list_t  *list   root element for user or list
 * int32_t  id;    uid or gid of caller
 *
 * return NULL if not found else the correspindinf
 * rfs_idmap_t *element
 *
 ***********************************************************/

static rfs_idmap_t *get_owner_entry(list_t *list, int32_t id)
{
     while(list)
     {
         if ( ((rfs_idmap_t*)(list->data))->id == id )
         {
             return (rfs_idmap_t*)list->data;
         }
         list = list->next;
     }

     return NULL;
}

/**********************************************************
 *
 * check_is_same()
 *
 * check if the name@host and name mean the same user/group
 * 
 *
 * int sock the socked handle for comminication
 *
 * return 1 if this is the same oener/group
 *
 ***********************************************************/

static int check_is_same(rfs_idmap_t *to_check, rfs_idmap_t *proc_owner)
{
    char *s = to_check   ? to_check->name   : NULL;
    char *t = proc_owner ? proc_owner->name : NULL;

    if ( s && t )
    {
        for(;;)
        {
            /* fist check if the host part begin if so
             * we have the same name
             */
            if ( *s == '@' && *t == '\0' )
            {
                return 1;
            }
            /* if  the actual characters differs we habe
             * different name so tell no the same.
             * if we have reached the end of one name
             * but not the other we will return here.
             */
            if ( *s != *t )
            {
                return 0;
            }
            else if ( *s == '\0' )
            {
                /*
                 * We must also take into considerations
                 * that an error within our code will produce
                 * 2 entries with the same name (shall not be
                 * possible) and we don't want to crash
                 */
                 return 1;
            }
            /* we can now check the next characters */
            s++;
            t++;
        }
    }
    return 0;
}

/**********************************************************
 *
 * process_message()
 *
 * get ans answer the messages from the resolver
 * library
 *
 * int sock the socked handle for comminication
 *
 * return 0 on error, 1 on success
 *
 ***********************************************************/

static int process_message(int sock)
{
    cmd_t command;
    list_t *list = NULL;
    rfs_idmap_t *owner_idmap_entry = NULL;
    /* get message */
    int ret = recv(sock, &command, sizeof(command), 0);

    if ( ret == -1 )
    {
        if (log ) perror("recv");
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
            remove_host(command.name, command.caller_id);
            if ( log ) printf("Number of rfs clients: %d\n", connections);
            return 1;
        break;

        case INC_CONN:
            connections++;
            add_host(command.name, command.caller_id);
            if ( log ) printf("Number of rfs clients: %d\n", connections);
            return 1;
        break;

        case GETPWENT:
        case GETGRENT:
            if ( log ) printf("Message received: %s\n",
                              command.cmd == GETPWENT
                                  ? "getpwent" : "getgrent");

            list = command.cmd == GETPWENT ? user_list: group_list;
            owner_idmap_entry = get_owner_entry(list, command.caller_id);
            command.name[0] = '\0';
            while(list)
            {
               rfs_idmap_t *map = (rfs_idmap_t*)list->data;
               if ( map->id > command.id && ! map->sys )
               {
                   /* if command.caller_id has the same name
                    * (without @host) we will ignore this entry
                    */
                   if ( ! check_is_same(map, owner_idmap_entry) )
                   {
                       break;
                   }

               }
               list = list->next;
            }

            if ( list )
            {
                /*  check if user has entry for the given host */
                if ( ! user_is_concerned((rfs_idmap_t*)list->data, command.caller_id ) )
                {
                    break;
                }
                command.found = 1;
                command.id    = ((rfs_idmap_t*)(list->data))->id;
                strncpy(command.name, ((rfs_idmap_t*)(list->data))->name,
                        sizeof(command.name));
            }

        break;

        case PUTGRNAM:
        case PUTPWNAM:
        case GETGRNAM:
        case GETPWNAM:
            if ( log ) printf("Message received: %s(%s)\n",
                                command.cmd == GETGRNAM
                              ? "getgrnam"
                              : command.cmd == GETPWNAM
                              ? "getpwnam"
                              : command.cmd == PUTPWNAM
                              ? "putpwnam"
                              : "putgrnam",
                              command.name);

            command.id    = 0;
            command.found = 0;
            if ( command.cmd == GETGRNAM || command.cmd == PUTGRNAM)
                list = group_list;
            else
                list = user_list;

            owner_idmap_entry = get_owner_entry(list, command.caller_id);
            /* search for given name */
            list = search_name(&command, list);

            /* if the rfs entry is misplaced (at the begin of the list
             * and we put automatically the login name into our list
             * we have to answer not found id we find an entry as root
             * in our list
             */
            if ( list && ((rfs_idmap_t*)(list->data))->sys )
            {
                if ( ! check_is_same((rfs_idmap_t*)(list->data), owner_idmap_entry) )
                {
                    command.found = 0;
                    command.id    = 0;
                    *command.name = '\0';
                }
                else
                {
                   command.found = 1;
                   command.id = owner_idmap_entry->id;
                   strcpy(command.name, owner_idmap_entry->name);
                }
                break;
            }

            if ( !(command.cmd == PUTGRNAM || command.cmd == PUTPWNAM) )
            {
                 break;
            }

            if (list == NULL )
            {
                if ( command.cmd == PUTGRNAM || command.cmd == GETGRNAM)
                    list = group_list;
                else if ( command.cmd == PUTPWNAM || command.cmd == GETPWNAM )
                    list = user_list;

                int hostid = get_hostid(command.name);
                /* the name was not found, insert it to the global list */
                insert_new_idmap(&list, &command, hostid);
            }

        break;

        case GETGRGID:
        case GETPWUID:
            if ( log ) printf("Message received: %s(%d)\n",
                              command.cmd == GETGRGID
                                  ? "getgrgid" : "getgruid",
                              command.id);
            command.name[0] = '\0';
            if ( command.cmd == GETGRGID )
                list = group_list;
            else
                list = user_list;
            /* search only the corresponding entry */
            list = search_id(&command, list);

            /* if the rfs entry is misplaced (at the begin of the list
             * and we put automatically the login name into our list
             * we have to answer not found id we find an entry as root
             * in our list
             */
            if ( list && ((rfs_idmap_t*)(list->data))->sys )
            {
                command.found = 0;
                command.id    = 0;
                *command.name = '\0';
                break;
            }
        break;

        default:
           printf("Command %d ?\n",command.cmd);
           return 0;
    }

    if ( log )
    {
        printf("    Answer: name = %s, id = %d found %d\n", command.name, command.id, command.found);
    }
    
    /* answer queries from library */
    send(sock, &command, sizeof(command),0 );
    return 1; /* all was OK */
}

/**********************************************************
 *
 * main_loop()
 *
 * wait for message from the resolver library
 * and process them
 *
 *
 *********************************************************/

static void main_loop(int sock)
{
    int ret;
    int accpt;
    struct sockaddr_un sock_address;
    socklen_t    sockLen = sizeof(sock_address);
    memset(&sock_address, 0, sizeof(sock_address));
    while(connections > 0)
    {
        if ( log )
        {
            printf("Wait for connection\n");
        }
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
               if ( log )
               {
                   printf("Accept connection\n");
               }
               ret = process_message(accpt);
               close(accpt);
               if ( log )
               {
                   printf("Message processed, connection closed\n");
               }
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

/***************************************************
 *
 * add_to_list()
 *
 * add the given data so that we have a sorted list
 *
 * list_t  **root   the root for our global list
 * char     *name   user or group name
 * uid_t     uid    or gif of user/group
 * int       is_sys tell if this is rom passed (1)
 *                  or was created by rfs_nss
 *
 * Return 0 on error else 1
 *
 ***************************************************/

static int add_to_list(list_t **root, char *name, uid_t id, int is_sys)
{
    int ret = 1;
    rfs_idmap_t *data = (rfs_idmap_t*) calloc(sizeof(rfs_idmap_t),1);
    if ( data == NULL )
    {
         if ( log ) perror("calloc");
         return 0;
    }

    data->id =  id;
    data->sys  = is_sys;
    data->name = strdup(name);
    if ( data->name == NULL )
    {
         if ( log ) perror("strdup");
        return 0;
    }

    if ( *root == NULL )
    {
       ret = list_insert(root, data, NULL);
    }
    else
    {
        list_t *elem = *root;
        while ( elem  )
        {
           if( ((rfs_idmap_t*)(elem->data))->id > id)
           {
                ret = list_insert(&elem, data, NULL);
                break;
           }
           else if ( elem->next == NULL )
           {
                ret = list_add(&elem, data, NULL);
                break;
           }
           elem = elem->next;
        }
    }
    return ret;
}

/***************************************
 *
 * syntax()
 *
 *
 **************************************/
static void syntax(char *prog_name)
{
     printf("Syntax: %s [-f] [-l] -s|e host \n", prog_name);
     printf("      -f, start in foreground\n");
     printf("      -l, print debug info.\n");
     printf("      -s  host  start and add name from host\n");
     printf("      -e  host  end and remove name for host\n");
     exit(1);
}

/***************************************
 *
 * main()
 *
 *
 **************************************/

int main(int argc,char **argv)
{
    struct passwd *pwd;
    struct group  *grp;
    int            sock = -1;
    int            ret  = 1;
    int            daemonize = 1;
    int            opt;
    char           *prog_name = strrchr(argv[0],'/');
    char           *ip_host = NULL;
    int             mode = 1; /* 1 = start, -1 = stop */
    int             check = 0;
    uid_t           uid = 0;
    char            host[NI_MAXHOST];
    int             nohost = 0;
    int             kill_rfs = 0;

    /* parse arguments */
    if ( prog_name == NULL )
        prog_name = argv[0];
    else
        prog_name++;
    while ( (opt = getopt(argc, argv, "fls:e:nk")) != -1 )
    {
        switch(opt)
        {
             case 'f': daemonize    = 0; break;
             case 'l': log          = 1; break;
             case 's': ip_host      = optarg; break;
             case 'e': ip_host      = optarg; mode = -1; break;
             case 'n': nohost       = 1; break;
             case 'k': kill_rfs     = 1; mode = -1; break;
             default: syntax(prog_name);
        }
    }

    *host = '\0';
    /* check first for running server */
    switch ((check = control_rfs_nss(CHECK_SERVER, NULL, NULL, NULL)))
    {
        case RFS_NSS_OK:
            /* a server is running, don't start */
            if ( nohost ) return 0;
            uid = getuid();
            if ( ip_host )
            {
                translate_ip(ip_host, host, NI_MAXHOST);
            }
            if ( mode == -1 )
            {
                do
                {
                    ret = control_rfs_nss(DEC_CONN, NULL, host, &uid);
                } while (kill_rfs && !(ret == RFS_NSS_NO_SERVER||ret == RFS_NSS_SYS_ERROR));
                return 0;
            }

            /* but add names from client if there is one */
            printf("An other server is allready running!\n");
            control_rfs_nss(INC_CONN, NULL, host, &uid);
            if ( mode == 1 && ip_host )
            {
                get_all_names(ip_host, host);
            }
            return 0;
        break;
        case RFS_NSS_SYS_ERROR:
        case RFS_NSS_NO_SERVER:
            /* no server detected ! */
            unlink(SOCKNAME);
        break;
    }

    if(mode == -1)
    {
        return 0;
    }

    if ( nohost == 0 && ip_host == NULL )
    {
        syntax(prog_name);
    }

    if ( nohost == 0 && kill_rfs == 0)
    {
       /* Add host */
       translate_ip(ip_host, host, NI_MAXHOST);
       add_host(host, getuid());
    }

    /* collect the known users */
    setpwent();
    ret = 1;
    while( ret &&(pwd = getpwent()) && ret )
    {
       if ( strchr(pwd->pw_name, '@') == NULL )
          ret = add_to_list(&user_list, pwd->pw_name, (uid_t)pwd->pw_uid, 1);
    }
    endpwent();

    if ( ret == 0 )
    {
       /* no enough memory ? we have a big problem and terminate */
       exit(1);
    }

    /* collect the known groups */
    setgrent();
    ret = 1;
    while(ret && (grp = getgrent()))
    {
       if ( strchr(grp->gr_name, '@') == NULL )
          ret = add_to_list(&group_list, grp->gr_name, (uid_t)grp->gr_gid, 1);
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
         if ( mode == 1 && ip_host )
         {
            if ( *host == '\0' )
            {
                translate_ip(ip_host, host, NI_MAXHOST);
            }
            get_all_names(ip_host, host);
         }
         return 0;
     }

     main_loop(sock);

     close(sock);
     unlink(SOCKNAME);
     unlink(PIDFILE);
     /* remove all allocated memory so we can check with valgrind for leaks,... */
     list_t *root = user_list;
     while(root)
     {
         free(((rfs_idmap_t*)root->data)->name);
         free(root->data);
         list_remove(&root,root);
     }
     root = group_list;
     while(root)
     {
         free(((rfs_idmap_t*)root->data)->name);
         free(root->data);
         list_remove(&root,root);
     }

    return 0;
}
