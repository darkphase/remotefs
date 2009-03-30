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
 * Function get_all_names
 * get all user and group name from the rfs client and
 * put them to the running rfs_nss server.
 *
 *****************************************************/
int get_all_names(char *ip_or_name)
{
   struct list *users  = NULL;
   struct list *user   = NULL;
   struct list *groups = NULL;
   struct list *group  = NULL;
   
   /* get and store all user names */
   if ( nss_get_users(ip_or_name, &users) == 0 )
   {
      user = users;
      while (user != NULL)
      {
         rfs_putpwnam((char *)user->data, ip_or_name);
         user = user->next;
      }
      destroy_list(&users);
   }

   /* get and store all group names */
   if ( nss_get_groups(ip_or_name, &groups) == 0 )
   {
      group = groups;
      while (group != NULL)
      {
         rfs_putgrnam((char *)group->data, ip_or_name);
         group = group->next;
      }
      destroy_list(&groups);
   }

   return 0;
}
