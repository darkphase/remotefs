#include "passwd.h"

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include "config.h"
#include "list.h"
#include "buffer.h"

#ifdef RFS_DEBUG
static const char *passwd_file = "./rfs-passwd";
#else
static const char *passwd_file = "/etc/rfs-passwd";
#endif /* RFS_DEBUG */
static struct list *auths = NULL;

int add_or_replace_auth(const char *user, const char *passwd)
{
	struct list *item = auths;
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
		struct auth_entry *auth = get_buffer(sizeof(struct auth_entry));
		auth->user = strdup(user);
		auth->passwd = strdup(passwd);
		
		struct list *added = add_to_list(auths, auth);
		if (added == NULL)
		{
			free(auth->user);
			free(auth->passwd);
			
			return -1;
		}
		
		if (auths == NULL)
		{
			auths = added;
		}
	}
	
	return 0;
}

int change_auth_password(const char *user, const char *passwd)
{
	struct list *item = auths;
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

const char *get_auth_password(const char *user)
{
	struct list *item = auths;

	while (item != NULL)
	{
		struct auth_entry *auth = (struct auth_entry *)(item->data);
		if (strcmp(auth->user, user) == 0)
		{
			return auth->passwd;
		}
		
		item = item->next;
	}
	
	return NULL;
}

int load_passwords()
{
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
				buffer[done] = ch;
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
			
			add_or_replace_auth(user, passwd);
		}
	}
	while (eof == 0);
	
	fclose(fp);
	
	return 0;
}

int save_passwords()
{
	FILE *fp = fopen(passwd_file, "wt");
	if (fp == NULL)
	{
		return -errno;
	}

	struct list *item = auths;
	
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

void release_passwords()
{
	struct list *item = auths;

	while (item != NULL)
	{
		struct auth_entry *auth = (struct auth_entry *)(item->data);
		free(auth->user);
		free(auth->passwd);
		
		item = item->next;
	}
	
	destroy_list(auths);
	auths = NULL;
}

int delete_auth(const char *user)
{
	struct list *item = auths;

	while (item != NULL)
	{
		struct auth_entry *auth = (struct auth_entry *)(item->data);
		if (strcmp(auth->user, user) == 0)
		{
			free(auth->user);
			free(auth->passwd);
			
			struct list *next = remove_from_list(item);
			if (item == auths)
			{
				auths = next;
			}
			
			return 0;
		}
		
		item = item->next;
	}
	
	return -1;
}

void dump_passwords()
{
#ifdef RFS_DEBUG
	DEBUG("%s\n", "dumping passwords");
	struct list *item = auths;
	while (item != NULL)
	{
		struct auth_entry *auth = (struct auth_entry *)item->data;
		
		DEBUG("user: %s, passwd: %s\n", auth->user, auth->passwd);
		
		item = item->next;
	}
#endif /* RFS_DEBUG */
}
