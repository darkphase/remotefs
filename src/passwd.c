/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "buffer.h"
#include "config.h"
#include "list.h"
#include "passwd.h"

int add_or_replace_auth(struct rfs_list **auths, const char *user, const char *passwd)
{
	struct rfs_list *item = *auths;
	struct auth_entry *exists = NULL;
	
	while (item != 0)
	{
		struct auth_entry *auth = (struct auth_entry *)(item->data);
		if (strcmp(auth->user, user) == 0)
		{
			exists = auth;
			break;
		}
		
		item = item->next;
	}
	
	if (exists != NULL)
	{
		char *saved_ptr = exists->passwd;
		exists->passwd = strdup(passwd);
		free(saved_ptr);
	}
	else
	{
		struct auth_entry *auth = malloc(sizeof(struct auth_entry));
		auth->user = strdup(user);
		auth->passwd = strdup(passwd);
		
		if (add_to_list(auths, auth) == NULL)
		{
			free(auth->user);
			free(auth->passwd);
			
			return -1;
		}
	}
	
	return 0;
}

int change_auth_password(struct rfs_list **auths, const char *user, const char *passwd)
{
	struct rfs_list *item = *auths;
	struct auth_entry *exists = NULL;
	
	while (item != 0)
	{
		struct auth_entry *auth = (struct auth_entry *)(item->data);
		if (strcmp(auth->user, user) == 0)
		{
			exists = auth;
			break;
		}
		
		item = item->next;
	}
	
	if (exists != NULL)
	{
		char *saved_ptr = exists->passwd;
		exists->passwd = strdup(passwd);
		free(saved_ptr);
	}
	else
	{
		return -1;
	}
	
	return 0;
}

const char *get_auth_password(const struct rfs_list *auths, const char *user)
{
	const struct rfs_list *item = auths;

	while (item != NULL)
	{
		const struct auth_entry *auth = (const struct auth_entry *)(item->data);
		if (strcmp(auth->user, user) == 0)
		{
			return auth->passwd;
		}
		
		item = item->next;
	}
	
	return NULL;
}

int load_passwords(const char *passwd_file, struct rfs_list **auths)
{
	DEBUG("loading passwords from %s\n", passwd_file);

	FILE *fp = fopen(passwd_file, "rt");
	if (!fp)
	{
		return 0;
	}
	
	char buffer[1024] = { 0 };
	unsigned char eof = 0;
	size_t string_number = 0;
	
	do
	{
		++string_number;
		
		memset(buffer, 0, sizeof(buffer));
		size_t done = 0;
		
		do
		{
			if (done >= sizeof(buffer))
			{
				fclose(fp);
				return -E2BIG;
			}
			
			int ch = fgetc(fp);
			
			if (ch != '\n' 
			&& ch != EOF)
			{
				buffer[done] = (char)ch;
				++done;
			}
			else
			{
				if (ch == EOF)
				{
					eof = 1;
				}
				break;
			}
		}
		while (1);
		
		if (done > 0)
		{
			buffer[done] = '\0';
			
			char *delim = strchr(buffer, ':');
			if (delim <= buffer)
			{
				fclose(fp);
				return -EINVAL;
			}
			
			*delim = 0;
			const char *user = buffer;
			const char *passwd = delim + 1;
			
			add_or_replace_auth(auths, user, passwd);
		}
	}
	while (eof == 0);
	
	fclose(fp);
	
	return 0;
}

int save_passwords(const char *passwd_file, const struct rfs_list *auths)
{
	DEBUG("saving passwords to %s\n", passwd_file);

	FILE *fp = fopen(passwd_file, "wt");
	if (fp == NULL)
	{
		return -errno;
	}

	const struct rfs_list *item = auths;
	
	while (item != NULL)
	{
		const struct auth_entry *auth = (const struct auth_entry *)(item->data);
		
		size_t user_len = strlen(auth->user);
		size_t passwd_len = strlen(auth->passwd);
		if (fwrite(auth->user, 1, user_len, fp) != user_len
		|| fwrite(":", 1, 1, fp) != 1
		|| fwrite(auth->passwd, 1, passwd_len, fp) != passwd_len
		|| fwrite("\n", 1, 1, fp) != 1)
		{
			fclose(fp);
			return -errno;
		}
		
		item = item->next;
	}
	
	fclose(fp);

	return 0;
}

void release_passwords(struct rfs_list **auths)
{
	struct rfs_list *item = *auths;

	while (item != NULL)
	{
		struct auth_entry *auth = (struct auth_entry *)(item->data);
		free(auth->user);
		free(auth->passwd);
		
		item = item->next;
	}
	
	destroy_list(auths);
	*auths = NULL;
}

int delete_auth(struct rfs_list **auths, const char *user)
{
	struct rfs_list *item = *auths;

	while (item != NULL)
	{
		struct auth_entry *auth = (struct auth_entry *)(item->data);
		if (strcmp(auth->user, user) == 0)
		{
			free(auth->user);
			free(auth->passwd);
			
			remove_from_list(auths, item);
			
			return 0;
		}
		
		item = item->next;
	}
	
	return -1;
}

#ifdef RFS_DEBUG
void dump_passwords(const struct rfs_list *auths)
{
	DEBUG("%s\n", "dumping passwords");
	const struct rfs_list *item = auths;
	while (item != NULL)
	{
		struct auth_entry *auth = (struct auth_entry *)item->data;
		
		DEBUG("user: %s, passwd: %s\n", auth->user, auth->passwd);
		
		item = item->next;
	}
}
#endif /* RFS_DEBUG */
