#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "id_lookup.h"

/**********************************************************
 * add_name
 *
 * const char  *name      name to add to our linear string array
 * char       **buffer    address of buffer for holding the names
 *                        is allocated and must be freed
 * int         *data_size actual size of name list
 *
 * return 0 on error, 1 if all is OK
 *********************************************************/

static inline int add_name(const char *name, char **buffer, int *data_size)
{
   int len = strlen(name) +1;
   if ( *buffer == NULL )
   {
      *buffer = malloc(len);
   }
   else
   {
      *buffer = realloc(*buffer, *data_size + len);
   }
   
   if ( *buffer )
   {
      strcpy((*buffer) + *data_size, name);
      *data_size += len;
      return 1;
   }
   else
   {
      return 0;
   }
}

/**********************************************************
 * terminate_data
 *
 * If previous operations was OK and there are name
 * within our data buffer add an empty string at the end
 *
 * int          ret          old status
 * const char  *datas        buffer with all names
 * int          *data_size   actual size of name list
 *
 * return 0 on error, 1 if all is OK
 *********************************************************/

static inline int terminate_data(int ret, char **datas, int *data_size)
{
	if ( ret && *datas != NULL )
	{
		ret = add_name("", datas, data_size);
	}
	else
	{
		*datas = (char*)calloc(1,1);
		if ( *datas == NULL )
		{
			*data_size = 0;
			return 0;
		}
		*data_size = 1;
	}
	return ret;
}

/**********************************************************
 * get_user_names
 *
 * get all user name beginning with the given minimum uid
 * and add them to our buffer
 * .
 * struct list **uids list for users
 * uid_t   min        lowest uid for which we stote names
 * char   *datas      buffer with all names
 * int    *data_size  actual size of name list
 *
 * return 0 on error, 1 if all is OK
 *********************************************************/

static inline int get_user_names(struct list **uids, int min, char **datas, int *data_size)
{
	int ret = 1;
	const char *name;
	while ( ret && uids && *uids )
	{
		name = get_user_name(uids, min);
		if ( name )
		{
			ret = add_name(name, datas, data_size);
		}
		else
		{
			*uids = NULL;
		}
	}
	return terminate_data(ret, datas, data_size);
}

/**********************************************************
 * get_group_names
 *
 * get all group name beginning with the given minimum gid
 * and add them to our buffer
 * .
 * struct list **gids list for groups
 * gid_t   min        lowest fid for which we stote names
 * char   *datas      buffer with all names
 * int    *data_size  actual size of name list
 *
 * return 0 on error, 1 if all is OK
 *********************************************************/

static inline int get_group_names(struct list **gids, int min, char **datas, int *data_size)
{
	int ret = 1;
	const char *name;
	while ( ret && gids && *gids)
	{
		name = get_group_name(gids, min);
		if ( name )
		{
			ret = add_name(name, datas, data_size);
		}
		else
		{
			*gids = NULL;
		}
	}
	return terminate_data(ret, datas, data_size);
}

static int _handle_nss(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
	char *data = NULL;
	int   data_len = 0;
	int ret = 0;
	struct list *uids = instance->id_lookup.uids;
	ret = get_user_names(&uids, instance->config.min_id, &data, &data_len);
	if ( ret )
	{
		struct list *gids = instance->id_lookup.gids;
		ret = get_group_names(&gids, instance->config.min_id, &data, &data_len);
	}
	struct answer ans = { cmd_nss, data_len, ret==1?0:-1, errno };
	if ( ret )
	{
		if (rfs_send_answer_data(&instance->sendrecv, &ans, data, data_len) == -1)
		{
			if ( data ) free(data);
			return -1;
		}
	}
	if ( data ) free(data);
	return 0;
}
