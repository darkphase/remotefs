/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "buffer.h"
#include "command.h"
#include "config.h"
#include "crypt.h"
#include "exports.h"
#include "id_lookup.h"
#include "instance_server.h"
#include "list.h"
#include "passwd.h"
#include "utils.h"

static int check_password(struct rfsd_instance *instance)
{
	const char *stored_passwd = get_auth_password(instance->passwd.auths, instance->server.auth_user);
	
	if (stored_passwd != NULL)
	{	
		char *check_crypted = passwd_hash(stored_passwd, instance->server.auth_salt);
		
		DEBUG("user: %s, received passwd: %s, stored passwd: %s, salt: %s, required passwd: %s\n", instance->server.auth_user, instance->server.auth_passwd, stored_passwd, instance->server.auth_salt, check_crypted);
		
		int ret = (strcmp(check_crypted, instance->server.auth_passwd) == 0 ? 0 : -1);
		free(check_crypted);
		
		return ret;
	}
	
	return -1;
}

int check_permissions(struct rfsd_instance *instance, const struct rfs_export *export_info, const char *client_ip_addr)
{
	if (client_ip_addr == NULL)
	{
		return -1;
	}

	const char *client_ip = client_ip_addr;

#ifdef WITH_IPV6
	/* check if it's not IPv4 connection came to IPv6 interface 
	and fix it back to IPv4 for correct authentication  */
	const char ipv4_to_ipv6_mapping[] = "::ffff:";
	if (strstr(client_ip_addr, ipv4_to_ipv6_mapping) == client_ip_addr)
	{
		client_ip += strlen(ipv4_to_ipv6_mapping);
	}
#endif

	DEBUG("client ip: %s\n", client_ip);

	struct list *user_entry = export_info->users;
	while (user_entry != NULL)
	{
		const struct user_rec *rec = (const struct user_rec *)user_entry->data;

#ifdef WITH_UGO
		// UGO exports can't be authenticated w/o username
		if ((export_info->options & OPT_UGO) != 0 
		&& (rec->username == NULL || instance->server.auth_user == NULL))
		{
			user_entry = user_entry->next;
			continue;
		}
#endif

		unsigned can_pass = 0;

		if (rec->network != NULL)
		{
			if (compare_netmask(client_ip, rec->network, rec->prefix_len) != 0)
			{
				can_pass = 1;
			}

#ifdef WITH_IPV6
			/* do double check because ::ffff:/80 might not be allowed in exports
			but actual IPv4 address might be */
			if (can_pass == 0 
			&& client_ip > client_ip_addr 
			&& compare_netmask(client_ip_addr, rec->network, rec->prefix_len) != 0)
			{
				can_pass = 1;
			}
#endif
		
			DEBUG("passed check for network %s/%u: %u\n", rec->network, rec->prefix_len, can_pass);
		}

		if (rec->username != NULL 
		&& strcmp(rec->username, ALL_ACCESS_USERNAME) == 0)
		{
			DEBUG("allowing access according to \"%s\" specified\n", ALL_ACCESS_USERNAME);
			can_pass = 1;
		}

		if (can_pass == 0 
		&& rec->username != NULL)
		{
			if (instance->server.auth_user != NULL
			&& instance->server.auth_passwd != NULL
			&& strcmp(rec->username, instance->server.auth_user) == 0
			&& check_password(instance) == 0)
			{
				can_pass = 1;
			}
			
			DEBUG("passed check for username %s: %u\n", rec->username, can_pass);
		}

		if (can_pass != 0)
		{
			return 0;
		}
		
		user_entry = user_entry->next;
	}
	
	DEBUG("%s\n", "access denied");
	return -1;
}

int generate_salt(char *salt, size_t max_size)
{
	const char al_set_begin = 'a';
	const char al_set_end = 'z';
	const char alu_set_begin = 'A';
	const char alu_set_end = 'Z';
	const char num_set_begin = '0';
	const char num_set_end = '9';
	const char *additional = "./";
	
	memset(salt, 0, max_size);
	
	size_t empty_len = strlen(EMPTY_SALT);
	
	if (empty_len >= max_size)
	{
		return -1;
	}
	
	memcpy(salt, EMPTY_SALT, empty_len);
	
	enum e_set { set_al = 0, set_alu, set_num, set_additional, set_max };
	
	int i; for (i = empty_len; i < max_size - 1; ++i)
	{
		char ch = '\0';
		
		switch (rand() % set_max)
		{
		case set_al:
			ch = al_set_begin + (rand() % (al_set_end - al_set_begin));
			break;
		case set_alu:
			ch = alu_set_begin + (rand() % (alu_set_end - alu_set_begin));
			break;
		case set_num:
			ch = num_set_begin + (rand() % (num_set_end - num_set_begin));
			break;
		case set_additional:
			ch = additional[rand() % strlen(additional)];
			break;
			
		default:
			memset(salt, 0, max_size);
			return -1;
		}
		
		salt[i] = ch;
	}

	salt[max_size - 1] = 0;
	
	return 0;
}
