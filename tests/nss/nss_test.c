
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "../../src/nss_client.h"
#include "../../src/list.h"
#include "../../src/config.h"

int main(int argc, char **argv)
{	
	if (argc < 2)
	{
		exit(1);
	}

	struct list *users = NULL;
	int get_users_ret = nss_get_users(argv[1], &users);

	INFO("getting users result: %s\n", strerror(-get_users_ret));

	if (get_users_ret == 0)
	{
		struct list *user = users;
		while (user != NULL)
		{
			INFO("user: %s\n", (const char *)user->data);

			user = user->next;
		}
	}

	destroy_list(&users);

	struct list *groups = NULL;
	int get_groups_ret = nss_get_groups(argv[1], &groups);

	INFO("getting groups result: %s\n", strerror(-get_groups_ret));

	if (get_groups_ret == 0)
	{
		struct list *group = groups;
		while (group != NULL)
		{
			INFO("group: %s\n", (const char *)group->data);

			group = group->next;
		}
	}

	destroy_list(&groups);

	if (argc < 3)
	{
		exit(1);
	}

	INFO("checking result for %s: %s\n", argv[2], strerror(-nss_check_user(argv[2])));
	if (argc > 3)
	{
		INFO("checking result for %s: %s\n", argv[3], strerror(-nss_check_group(argv[3])));
	}


	return 0;
}


