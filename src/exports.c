#include "exports.h"

#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>

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
extern struct rfsd_config rfsd_config;

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
	
	while (local_buffer[0] == ' ' 
	|| local_buffer[0] == '\t')
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

int set_export_opts(struct rfs_export *opts_export, const struct list *opts)
{
	int ret = 0;
	
	const struct list *opt = opts;
	while (opt)
	{
		const char *opt_str = opt->data;
		
		if (strlen(opt_str) > 0)
		{
			if (strcmp(opt_str, "ro") == 0)
			{
				opts_export->options |= opt_ro;
			}
			else if (strstr(opt_str, "user=") == opt_str)
			{
				if (opts_export->export_uid != (uid_t)-1)
				{
					ERROR("%s", "User option for export is set twice\n");
					return -1;
				}
			
				const char *username = opt_str + strlen("user=");
				DEBUG("export username: %s\n", username);
				
				struct passwd *pwd = getpwnam(username);
				if (pwd == NULL)
				{
					ERROR("User %s is not found in *system* users database\n", username);
					return -1;
				}
				
				opts_export->export_uid = pwd->pw_uid;
			}
			else
			{
				ERROR("Unknown export option: %s\n", opt_str);
				return -1;
			}
		}
		
		opt = opt->next;
	}
	
	return ret;
}

struct list* parse_list(const char *buffer, const char *border)
{
	struct list *ret = NULL;
	
	const char *local_buffer = buffer;
	do
	{
		char *delimiter = strchr(local_buffer, ',');
		
		if (delimiter >= border)
		{
			break;
		}
		
		int len = (delimiter != NULL ? delimiter - local_buffer : border - local_buffer);
		
		const char *item_end = trim_right(delimiter != NULL ? delimiter - 1 : border - 1, len - 1);
		
		len = item_end - local_buffer;
		
		char *item = get_buffer(len + 1);
		memcpy(item, local_buffer, len);
		item[len] = 0;
		
		struct list *added = add_to_list(ret, item);
		if (added == NULL)
		{
			destroy_list(ret);
			return NULL;
		}
		else
		{
			if (ret == NULL)
			{
				ret = added;
			}
		}
		
		if (delimiter == NULL)
		{
			break;
		}
		
		local_buffer = trim_left(delimiter + 1, border - local_buffer);
	}
	while (local_buffer < border);
	
	return ret;
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
	
	const char *share_end = find_chr(local_buffer, next_line, "\t ", 2);
	if (share_end == NULL)
	{
		return (char *)-1;
	}
	
	unsigned share_len = share_end - local_buffer;
	char *share = get_buffer(share_len + 1);
	memset(share, 0, share_len + 1);
	memcpy(share, local_buffer, share_len);
	
	while (strlen(share) > 1 // do not remove first '/'
	&& share[strlen(share) - 1] == '/')
	{
		share[strlen(share) - 1] = 0;
	}
	
	if (strlen(share) < 1)
	{
		free_buffer(share);
		return (char *)-1;
	}
	
	const char *users = trim_left(share_end, next_line - local_buffer);
	const char *users_end = find_chr(users, next_line + 1, "(\n", 2);
	
	if (users == NULL 
	|| users >= next_line
	|| users_end == NULL
	|| users_end > next_line)
	{
		free_buffer(share);
		return (char *)-1;
	}
	
	struct list *this_line_users = parse_list(users, users_end);
	
	const char *options = find_chr(users_end, next_line + 1, "(", 1);
	struct list *this_line_options = NULL;
	
	if (options != NULL)
	{
		const char *options_end = find_chr(options, next_line + 1, ")", 1);
		if (options_end != NULL)
		{
			this_line_options = parse_list(options + 1, options_end);
		}
	}
	
	if (set_export_opts(line_export, this_line_options) != 0)
	{
		free_buffer(share);
		destroy_list(this_line_users);
		destroy_list(this_line_options);
		return (char *)-1;
	}
	
	if (this_line_options != NULL)
	{
		destroy_list(this_line_options);
	}
	
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
		line_export->export_uid = (uid_t)-1;
		
		next_line = parse_line(buffer, (unsigned)((buffer + size) - next_line), next_line - buffer, line_export);
		if (next_line == (char *)-1)
		{
			free_buffer(line_export);
			fclose(fd);
			return -1;
		}
		
		if (line_export->export_uid == (uid_t)-1)
		{
			line_export->export_uid = rfsd_config.worker_uid;
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

	struct list *user = single_export->users;
	DEBUG("%s%s", "users: ", user == NULL ? "\n" : "");
	while (user != NULL)
	{
		const char *username = (char *)user->data;
		
		DEBUG("'%s' (%s)%s", 
		username, 
		is_ipaddr(username) ? "ip address" : "username", 
		user->next == NULL ? "\n" : ", ");
		
		user = user->next;
	}

	DEBUG("options: %d\n", single_export->options);
	DEBUG("export uid: %d\n", single_export->export_uid);
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
