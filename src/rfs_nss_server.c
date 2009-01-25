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
#include <grp.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>

#include "rfs_nss.h"
#include "dllist.h"

int rfs_put_name = 0; /* main working mode for the server
                       * 0 = server generate entries on getXnam()
                       *     request
                       * 1 = client must send special command
                       *     for generatimg of entries
                       */

typedef struct rfs_idmap_s
{
    char *name;
    uid_t id;
    int sys;
} rfs_idmap_t;

static list_t *user_list = NULL;
static list_t *group_list = NULL;
static int     log = 0;
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

static int insert_new_idmap(list_t **root, cmd_t *command)
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
            if ( log ) printf("Number of rfs clients: %d\n", connections);
            return 1;
        break;

        case INC_CONN:
            connections++;
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

            /* search for given name */
            list = search_name(&command, list);

            if ( rfs_put_name && !(command.cmd == PUTGRNAM || command.cmd == PUTPWNAM) )
            {
                 break;
            }

            if (list == NULL )
            {
                if ( command.cmd == PUTGRNAM || command.cmd == GETGRNAM)
                    list = group_list;
                else if ( command.cmd == PUTPWNAM || command.cmd == GETPWNAM )
                    list = user_list;

                /* the name was not found, insert it to the global list */
                insert_new_idmap(&list, &command);
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
                list = group_list;
            else
                list = user_list;
            /* search only the corresponding entry */
            search_id(&command, list);
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

/***************************************************
 *
 * open_file()
 *
 * Open a file for saving or reding of the entries
 * created by rfs_nss
 * char *mode "r" for read, "w" for write
 *
 * Return a FILE poiunter or NULL
 *
 ***************************************************/

FILE *open_file(char *mode)
{
    char *db_file = ".rfs_nss";
    char *dir = NULL;
    char *path = NULL;
    uid_t uid = getuid();
    FILE *fp = NULL;
    
    if ( uid != 0 )
    {
        struct passwd *pwd = getpwuid(uid);
        if ( pwd != NULL && pwd->pw_dir != NULL )
        {
            dir = pwd->pw_dir;
        }
    }
    else
    {
        dir = "/var/rfs_nss";
        db_file = "rfs_nss";
        if ( access(dir, W_OK) == -1 )
        {
           mkdir(dir, 0700);
        }
    }

    if ( dir )
    {
        path = (char*)calloc(strlen(db_file)+strlen(dir)+2,1);
        if ( path == NULL )
        {
            perror("calloc");
            return NULL;
        }
        sprintf(path, "%s/%s", dir, db_file);
        fp = fopen(path,mode);
        free(path);
    }
    return fp;
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
    int             save = 0;
    FILE           *fp = NULL;
    list_t         *list = NULL;
    list_t         *root = NULL;

    /* parse arguments */
    if ( prog_name == NULL )
        prog_name = argv[0];
    else
        prog_name++;

    while ( (opt = getopt(argc, argv, "flrs")) != -1 )
    {
        switch(opt)
        {
             case 'f': daemonize    = 0; break;
             case 'l': log          = 1; break;
             case 'r': rfs_put_name = 1; break;
             case 's': save         = 1; break;
             default:
                printf("Syntax: %s [-f] [-l] [-r]\n", prog_name);
                printf("      -f, start in foreground\n");
                printf("      -l, print debug info.\n");
                printf("      -r, name send from rfs\n");
                printf("      -s, store db in a file\n");
                return 0;
        }
    }

    /* check first for running server */
    switch (control_rfs_nss(CHECK_SERVER, NULL, NULL, NULL))
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
    
    /* collect the known users */
    setpwent();
    ret = 1;
    while( ret &&(pwd = getpwent()) && ret )
    {
       ret = add_to_list(&user_list, pwd->pw_name, (uid_t)pwd->pw_uid, 1);
    }
    endpwent();

    if ( ret == 0 )
    {
       /* no enough memory ? we have a big problem and terminate */
       exit(1);
    }

    /* collect the known gtoups */
    setgrent();
    ret = 1;
    while(ret && (grp = getgrent()))
    {
       ret = add_to_list(&group_list, grp->gr_name, (uid_t)grp->gr_gid, 1);
    }
    endgrent();

    if ( ret == 0 )
    {
       /* no enough memory ? we have a big problem and terminate */
       exit(1);
    }

    if ( save )
    {
        cmd_t command;
        fp = open_file("r");
        if ( fp != NULL )
        {
            char line[1024];
            char name[1024];
            char type;
            uid_t id = 0;
            int   ok = 1;
            while (ok && fgets(line, sizeof(line),fp) )
            {
                if ( sscanf(line, "%c:%[^:]:%d", &type, name, &id) == 3 )
                {
                    if ( id < 10000 || id > 20000 )
                    {
                       ok = 0;
                    }
                    strncpy(command.name, name, sizeof(command.name));
                    command.name[sizeof(command.name)-1] = '\0';
                    switch(type)
                    {
                        case 'u': root = user_list; break;
                        case 'g': root = group_list; break;
                        default:  ok = 0; break;
                    }
                    if ( ok && (list = search_name(&command, root)) == NULL )
                    {
                        ok = insert_new_idmap(&root, &command);
                    }
                    if ( log && ok && id != command.id )
                    {
                        printf("%s ID old %d new %d\n",
                               name,
                               id,
                               command.id);
                    }
                }
            }
            fclose(fp);
            if (!ok && log)
            {
               printf("wrong line %s\n",line);
            }
        }
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

     main_loop(sock);

     close(sock);
     unlink(SOCKNAME);
     unlink(PIDFILE);

    if ( save )
    {
        fp = open_file("w");
        if ( fp != NULL )
        {
            list_t *list = user_list;
            while (list)
            {
                rfs_idmap_t *idmap;
                idmap = (rfs_idmap_t*)list->data;
                if ( idmap->sys == 0 )
                {
                    fprintf(fp,"u:%s:%d\n", idmap->name, idmap->id);
                }
                list = list->next;
            }

            list = group_list;
            while (list)
            {
                rfs_idmap_t *idmap;
                idmap = (rfs_idmap_t*)list->data;
                if ( idmap->sys == 0 )
                {
                    fprintf(fp, "g:%s:%d\n", idmap->name, idmap->id);
                }
                list = list->next;
            }
            fclose(fp);
        }
        else
        {
             perror("fopen");
        }
    }
    return 0;
}
