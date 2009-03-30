/********************************************
 * File: rfs_getnames.c
 *
 * ask the rfs cleint for user and group name
 *
 *******************************************/

/* OS includes */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#if defined FREEBSD
#include <netinet/in.h>
#endif
#if defined QNX
#include <arpa/inet.h>
#endif
#include <pwd.h>
#include <grp.h>

/* rfs_nss includes */
#include "rfs_nss.h"

/* remotefs includes */
#include <nss_client.h>
#include <list.h>

/******************************************************
 * Function translate_ip
 * convert the ip to a host name.
 *
 *****************************************************/

int translate_ip(char *ip_or_name, char *host, size_t hlen)
{
    struct addrinfo *addr_info = NULL;
    struct addrinfo hints = { 0 };

    /* clear host */
    if ( host )
    {
       *host = '\0';
    }

    /* resolve name or address */
    hints.ai_family    = AF_UNSPEC;
    hints.ai_socktype  = SOCK_STREAM; 
    hints.ai_flags     = AI_ADDRCONFIG | AI_CANONNAME;
#if defined QNX
     hints.ai_flags     = 0;
#endif
    /* get host address */
    int result = getaddrinfo(ip_or_name, NULL, &hints, &addr_info);
    if (result != 0)
    {
         fprintf(stderr,"Can't resolve address: %s\n", gai_strerror(result));
    }
    else if ( host )
    {
        getnameinfo((struct sockaddr *)addr_info->ai_addr,
                     addr_info->ai_addrlen, host, hlen, NULL, 0, 0);
        freeaddrinfo(addr_info);
    }

    /* if name resolving was not OK copy the name/ip to host */
    if (  host && *host == '\0' )
    {
        strncpy(host, ip_or_name, hlen);
        host[hlen-1] = '\0';
    }

    return 0;
}

/******************************************************
 * Function get_all_names
 * get all user and group name from the rfs client and
 * put them to the running rfs_nss server.
 *
 *****************************************************/
int get_all_names(char *ip_or_name, char *host)
{
   struct list *users  = NULL;
   struct list *user   = NULL;
   struct list *groups = NULL;
   struct list *group  = NULL;
   struct passwd *pwd;
   struct group *grp;
   
   /* get and store all user names */
   if ( nss_get_users(ip_or_name, &users) == 0 )
   {
      user = users;
      while (user != NULL)
      {
         pwd = getpwnam((char *)user->data);
         rfs_putpwnam((char *)user->data, host);
         user = user->next;
      }
      destroy_list(&users);
   }

   /* get and store all group names */
   if ( nss_get_groups(ip_or_name, &groups) == 0 )
   {
      while (group != NULL)
      {
         group = groups;
         grp = getgrnam((char *)group->data);
         rfs_putgrnam((char *)user->data, host);
         group = groups->next;
      }
      destroy_list(&groups);
   }

   return 0;
}
