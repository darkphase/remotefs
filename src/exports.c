#include "exports.h"

#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>

#include "config.h"
#include "buffer.h"
#include "list.h"

#ifdef RFS_DEBUG
static const char *exports_file = "./rfs-exports";
#else
static const char *exports_file = "/etc/rfs-exports";
#endif // RFS_DEBUG
static struct list *exports = NULL;

void release_export(struct rfs_export *single_export);

const char* trim_left(const char *buffer, unsigned size)
{
	const char *local_buffer = buffer;
	unsigned skipped = 0;
	
	while (local_buffer[0] == ' ' || local_buffer[0] == '\t')
	{
		++local_buffer;
		++skipped;
		
		if (skipped > size)
		{
			return local_buffer;
		}
	}
	
	return local_buffer;
}

const char* trim_right(const char *buffer, unsigned size)
{
	const char *local_buffer = buffer;
	unsigned skipped = 0;
	
	while (local_buffer[0] == ' ' || local_buffer[0] == '\t')
	{
		--local_buffer;
		++skipped;
		
		if (skipped > size)
		{
			return local_buffer + 1;
		}
	}
	
	return local_buffer + 1;
}

const char* find_chr(const char *buffer, const char *border, const char *symbols, unsigned symbols_count)
{
	const char *min_ptr = NULL;
	int i = 0; for (i = 0; i < symbols_count; ++i)
	{
		const char *ptr = strchr(buffer, symbols[i]);
		if (ptr != NULL && ptr < border)
		{
			if (min_ptr == NULL)
			{
				min_ptr = ptr;
			}
			else
			{
				if (ptr < min_ptr)
				{
					min_ptr = ptr;
				}
			}
		}
	}
	
	return min_ptr;
}

unsigned is_ipaddr(const char *string)
{
	return inet_addr(string) == INADDR_NONE ? 0 : 1;
}

char* parse_line(const char *buffer, unsigned size, int start_from, struct rfs_export *line_export)
{
	const char *local_buffer = buffer + start_from;
	char *next_line = strchr(local_buffer, '\n');
	
	local_buffer = trim_left(local_buffer, next_line - local_buffer);
	
	if (local_buffer == NULL 
	|| local_buffer[0] == '\n'
	|| local_buffer[0] == 0
	|| local_buffer[0] == '#')
	{
		return next_line == NULL ? NULL : next_line + 1;
	}
	
	struct list *this_line_users = NULL;
	
	const char *share_end = find_chr(local_buffer, next_line, "\t ", 2);
	if (share_end == NULL)
	{
		return (char *)-1;
	}
	
	unsigned share_len = share_end - local_buffer;
	char *share = get_buffer(share_len + 1);
	memset(share, 0, share_len + 1);
	memcpy(share, local_buffer, share_len);
	
	const char *users = trim_left(share_end, next_line - local_buffer);
	if (users == NULL || users >= next_line)
	{
		free_buffer(share);
		return (char *)-1;
	}
	
	do
	{
		const char *user_end = find_chr(users, next_line + 1, ",\n", 2);
		if (user_end == NULL || user_end > next_line)
		{
			return (char *)-1;
		}
		
		unsigned user_len = trim_right(user_end - 1, user_end - users - 1) - users;
		char *user = get_buffer(user_len + 1);
		memset(user, 0, user_len + 1);
		memcpy(user, users, user_len);
		
		struct list *added = add_to_list(this_line_users, user);
		if (this_line_users == NULL)
		{
			this_line_users = added;
		}
		
		users = trim_left(user_end + 1, next_line - user_end + 1);
	}
	while (users < next_line && users != 0);
	
	line_export->path = share;
	line_export->users = this_line_users;

	return next_line == NULL ? NULL : next_line + 1;
}

unsigned parse_exports()
{
	FILE *fd = fopen(exports_file, "rt");
	if (fd == 0)
	{
		return -1;
	}
	
	if (fseek(fd, 0, SEEK_END) != 0)
	{
		fclose(fd);
		return -1;
	}
	
	long size = ftell(fd);
	
	char *buffer = get_buffer(size + 1);
	memset(buffer, 0, size + 1);
	
	if (buffer == NULL)
	{
		fclose(fd);
		return -1;
	}
	
	if (fseek(fd, 0, SEEK_SET) != 0)
	{
		free_buffer(buffer);
		fclose(fd);
		return -1;
	}
	
	if (fread(buffer, 1, size, fd) != size)
	{
		free_buffer(buffer);
		fclose(fd);
		return -1;
	}
	
	char *next_line = buffer;
	do
	{
		struct rfs_export *line_export = get_buffer(sizeof(struct rfs_export));
		
		next_line = parse_line(buffer, (unsigned)((buffer + size) - next_line), next_line - buffer, line_export);
		if (next_line == (char *)-1)
		{
			free_buffer(line_export);
			fclose(fd);
			return -1;
		}
		
		if (line_export->path != 0 && line_export->users != 0)
		{
			struct list *added = add_to_list(exports, line_export);
			if (exports == NULL)
			{
				exports = added;
			}
		}
		else
		{
			free_buffer(line_export);
		}
	}
	while (next_line != NULL);
	
	free_buffer(buffer);
	
	fclose(fd);
	
	return 0;
}

void release_export(struct rfs_export *single_export)
{
	free_buffer(single_export->path);
	destroy_list(single_export->users);
	single_export->users = NULL;
}

void release_exports()
{
	struct list *single_export = exports;
	while (single_export != NULL)
	{
		struct list *next = single_export->next;
		release_export(single_export->data);
		single_export = next;
	}
	destroy_list(exports);
	exports = NULL;
}

void dump_export(const struct rfs_export *single_export)
{
#ifdef RFS_DEBUG
	DEBUG("%s", "dumping export:\n");
	DEBUG("path: '%s'\n", single_export->path);
	DEBUG("%s", "users: ");

	struct list *user = single_export->users;
	while (user != NULL)
	{
		const char *username = (char *)user->data;
		
		DEBUG("'%s' (%s)%s", 
		username, 
		is_ipaddr(username) ? "ip address" : "username", 
		user->next == NULL ? "\n" : ", ");
		
		user = user->next;
	}
#endif // RFS_DEBUG
}

const struct rfs_export* get_export(const char *path)
{
	struct list *single_export = exports;
	while (single_export != NULL)
	{
		const struct rfs_export *export_data = (const struct rfs_export *)single_export->data;
		if (strcmp(export_data->path, path) == 0)
		{
			return export_data;
		}
		
		single_export = single_export->next;
	}
	
	return NULL;
}

void dump_exports()
{
	DEBUG("%s", "dumping exports set:\n");
	struct list *single_export = exports;
	while (single_export != NULL)
	{
		dump_export(single_export->data);
		single_export = single_export->next;
	}
}
