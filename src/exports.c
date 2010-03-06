/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <string.h>
#include <grp.h>
#include <pwd.h>
#include <errno.h>

#include "buffer.h"
#include "config.h"
#include "exports.h"
#include "list.h"
#include "utils.h"

#ifdef RFS_DEBUG
static void dump_export(const struct rfs_export *single_export);
#endif /* RFS_DEBUG */

static const char* trim_left(const char *buffer, unsigned size)
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

static const char* trim_right(const char *buffer, unsigned size)
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

static const char* find_chr(const char *buffer, const char *border, const char *symbols)
{
	const char *min_ptr = NULL;
	int i = 0; for (i = 0; i < strlen(symbols); ++i)
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

static int set_export_opts(struct rfs_export *opts_export, const struct list *opts)
{
	const struct list *opt = opts;
	while (opt)
	{
		const char *opt_str = opt->data;
		
		if (strlen(opt_str) > 0)
		{
			if (strcmp(opt_str, "ro") == 0)
			{
				opts_export->options |= OPT_RO;
			}
#ifdef WITH_UGO
			else if (strcmp(opt_str, "ugo") == 0)
			{
				opts_export->options |= OPT_UGO;
			}
#endif
			else if (strstr(opt_str, "user=") == opt_str)
			{
				if (opts_export->export_uid != (uid_t)-1)
				{
					ERROR("%s", "User option for export is set twice\n");
					return -1;
				}
				
				const char *export_username = opt_str + strlen("user=");
				
				struct passwd *pwd = getpwnam(export_username);
				if (pwd == NULL)
				{
					ERROR("User %s is not found in *system* users database\n", export_username);
					return -1;
				}
				
				opts_export->export_uid = pwd->pw_uid;
			}
			else
			{
				ERROR("Unknown export option: \"%s\"\n", opt_str);
				return -1;
			}
		}
		
		opt = opt->next;
	}
	
	return 0;
}

struct list* parse_list(const char *buffer, const char *border)
{
	struct list *ret = NULL;
	
	const char *local_buffer = buffer;
	do
	{
		const char *delimiter = find_chr(local_buffer, border, ",()\n");
		
		if (delimiter > border)
		{
			break;
		}
		
		local_buffer = trim_left(local_buffer, delimiter - local_buffer);
		
		int len = (delimiter != NULL ? delimiter - local_buffer : border - local_buffer);
		
		const char *item_end = trim_right(delimiter != NULL ? delimiter - 1 : border - 1, len - 1);
		
		len = item_end - local_buffer;
		
		char *item = malloc(len + 1);
		memcpy(item, local_buffer, len);
		item[len] = 0;
		
		if (add_to_list(&ret, item) == NULL)
		{
			destroy_list(&ret);
			return NULL;
		}
		
		if (delimiter == NULL)
		{
			break;
		}
		
		local_buffer = delimiter + 1;
	}
	while (local_buffer < border);
	
	return ret;
}

static inline unsigned default_prefix_len(const char *ip_addr)
{
	DEBUG("defaulting prefix length for %s\n", ip_addr);

#ifdef WITH_IPV6
	if (strchr(ip_addr, ':') != NULL)
	{
		return 128;
	}
	else
#endif
	{
		return 32;
	}
}

static struct list* parse_users(const struct list *users_list)
{
	struct list *fixed_users = NULL;

	const struct list *item = users_list;
	while (item != NULL)
	{
		const char *id = (const char *)item->data;
		struct user_rec *rec = malloc(sizeof(*rec));

		rec->username = NULL;
		rec->network = NULL;
		rec->prefix_len = (unsigned)-1;

		DEBUG("%s\n", id);

		const char *prefix_delimiter = strchr(id, '/');
		const char *network_delimiter = strchr(id, '@');

		if (prefix_delimiter != NULL 
		&& prefix_delimiter - id != strlen(id))
		{
			rec->prefix_len = atoi(prefix_delimiter + 1);
		}

		if (network_delimiter != NULL) /* it's something like root@127.0.0.1 */
		{
			rec->username = buffer_dup_str(id, network_delimiter - id);
			rec->network = buffer_dup_str(network_delimiter + 1, 
				prefix_delimiter != NULL 
				? prefix_delimiter - network_delimiter - 1 
				: strlen(id) - (network_delimiter + 1 - id));

			if (prefix_delimiter == NULL)
			{
				rec->prefix_len = default_prefix_len(rec->network);
			}

			DEBUG("%s %s\n", rec->username, rec->network);
		}
		else if (prefix_delimiter == NULL) /* no network delimiter and no prefix length */
		{
			if (is_ipaddr(id) != 0)
			{
				rec->network = buffer_dup(id, strlen(id) + 1);
				rec->prefix_len = default_prefix_len(id);
			}
			else
			{
				rec->username = buffer_dup(id, strlen(id) + 1);
			}
		}
		else /* prefix delimiter ('/') is present, but no network delimiter ('@') */
		{
			rec->network = buffer_dup_str(id, prefix_delimiter - id);

			if (is_ipaddr(rec->network) == 0)
			{
				goto error;
			}
		}
		
		add_to_list(&fixed_users, rec);
		
		item = item->next;
	}

	return fixed_users;

error:

	{

	struct list *fixed_item = fixed_users;
	while (fixed_item != NULL)
	{
		struct user_rec *rec = (struct user_rec *)fixed_item->data;

		if (rec->username != NULL)
		{
			free(rec->username);
		}

		if (rec->network != NULL)
		{
			free(rec->network);
		}

		fixed_item = fixed_item->next;
	}

	destroy_list(&fixed_users);
	
	return (struct list *)(-1);

	}
}

static char* parse_line(const char *buffer, unsigned size, struct rfs_export *line_export)
{
	const char *local_buffer = buffer;
	char *next_line = strchr(local_buffer, '\n');
	const char *border = (next_line != NULL ? next_line : local_buffer + strlen(local_buffer));
	
	local_buffer = trim_left(local_buffer, border - local_buffer);
	
	if (local_buffer == NULL 
	|| local_buffer[0] == '\n'
	|| local_buffer[0] == 0
	|| local_buffer[0] == '#')
	{
		return next_line == NULL ? NULL : next_line + 1;
	}
	
	const char *share_end = find_chr(local_buffer, border, "\t ");
	if (share_end == NULL)
	{
		return (char *)-1;
	}
	
	unsigned share_len = share_end - local_buffer;
	char *share = malloc(share_len + 1);
	memset(share, 0, share_len + 1);
	memcpy(share, local_buffer, share_len);
	
	while (strlen(share) > 1 /* do not remove first '/' */
	&& share[strlen(share) - 1] == '/')
	{
		share[strlen(share) - 1] = 0;
	}
	
	if (strlen(share) < 1)
	{
		free(share);
		return (char *)-1;
	}
	
	const char *users = trim_left(share_end, border - local_buffer);
	const char *users_end = find_chr(users, border, "()");
	
	if (users_end == NULL)
	{
		users_end = next_line;
	}
	
	if (users == NULL 
	|| users >= border 
	|| users_end == NULL 
	|| users_end > border 
	|| users == users_end)
	{
		free(share);
		return (char *)-1;
	}

	struct list *users_list = parse_list(users, users_end);
	struct list *this_line_users = (users_list == NULL ? NULL : parse_users(users_list));
	destroy_list(&users_list);

	if (this_line_users == (struct list *)(-1))
	{
		return (char *)(-1);
	}

	const char *options = find_chr(users_end, border, "(");
	struct list *this_line_options = NULL;
	
	if (options != NULL)
	{
		const char *options_end = find_chr(options, border, ")");
		if (options_end != NULL)
		{
			this_line_options = parse_list(options + 1, options_end);
			
			if (set_export_opts(line_export, this_line_options) != 0)
			{
				goto parse_error;
			}
			
			if (this_line_options != NULL)
			{
				destroy_list(&this_line_options);
			}
		}
		else
		{
			goto parse_error;
		}

		/* check if there is nothing else after options list */
		if (options_end + 1 < border 
		&& trim_left(options_end + 1, border - (options_end + 1)) != border)
		{
			goto parse_error;
		}
	}
	
	line_export->path = share;
	line_export->users = this_line_users;
	
	return next_line == NULL ? NULL : next_line + 1;

parse_error:
	free(share);
	destroy_list(&this_line_users);
	destroy_list(&this_line_options);
	return (char *)-1;
}

static int validate_export(const struct rfs_export *line_export, struct list *exports)
{
#ifdef WITH_UGO
	if ((line_export->options & OPT_UGO) != 0)
	{
		if ((line_export->options & OPT_RO) != 0
		|| line_export->export_uid != -1)
		{
			ERROR("Export validation error: you can't specify \"ro\" or \"user=\" options simultaneously "
			"with \"ugo\" option. \"ugo\" will handle all security related issues for this export (%s).\n", 
			line_export->path);
			return -1;
		}
		
		struct list *user_item = line_export->users;
		while (user_item != NULL)
		{
			const struct user_rec *rec = (const struct user_rec *)user_item->data;
			if (rec->username == NULL)
			{
				ERROR("Export validation error: you can't authenticate user by IP-addresses "
				"while using \"ugo\" option for the same export (%s).\n", 
				line_export->path);
				return -1;
			}
			
			user_item = user_item->next;
		}
	}
#endif

	struct list *user_item = line_export->users;
	while (user_item != NULL)
	{
		struct user_rec *rec = (struct user_rec *)user_item->data;
		if (rec->network != NULL)
		{
			unsigned too_long_prefix = 0;
#ifdef WITH_IPV6
			if (strchr(rec->network, ':') != NULL)
			{
				if (rec->prefix_len > 128)
				{
					too_long_prefix = 1;
				}
			}
			else
#endif
			if (rec->prefix_len > 32)
			{
				too_long_prefix = 1;
			}

			if (too_long_prefix != 0)
			{
				ERROR("Too long prefix length (%u) for network %s\n", rec->prefix_len, rec->network);
				return -1;
			}
		}

		user_item = user_item->next;
	}

	if (line_export->path != NULL)
	{
		struct list *item = exports;
		while (item != NULL)
		{
			struct rfs_export *export = (struct rfs_export *)item->data;
			if (strcmp(line_export->path, export->path) == 0)
			{
				ERROR("Duplicate export: %s\n", export->path);
				return -1;
			}

			item = item->next;
		}
	}

	return 0;
}

int parse_exports(const char *exports_file, struct list **exports, uid_t worker_uid)
{
	FILE *fd = fopen(exports_file, "rt");
	if (fd == 0)
	{
		return -errno;
	}
	
	if (fseek(fd, 0, SEEK_END) != 0)
	{
		int saved_errno = errno;

		fclose(fd);
		return -saved_errno;
	}
	
	long size = ftell(fd);
	
	char *buffer = malloc(size + 1);
	memset(buffer, 0, size + 1);
	
	if (buffer == NULL)
	{
		int saved_errno = errno;

		fclose(fd);
		return -saved_errno;
	}
	
	if (fseek(fd, 0, SEEK_SET) != 0)
	{
		int saved_errno = errno;

		free(buffer);
		fclose(fd);
		return -saved_errno;
	}
	
	if (fread(buffer, 1, size, fd) != size)
	{
		int saved_errno = errno;
		
		free(buffer);
		fclose(fd);
		return -saved_errno;
	}

	int line_number = 0;
	char *next_line = buffer;
	do
	{
		++line_number;

		struct rfs_export *line_export = malloc(sizeof(struct rfs_export));
		memset(line_export, 0, sizeof(*line_export));
		
		line_export->export_uid = (uid_t)-1;
		
		next_line = parse_line(next_line, (buffer + size) - next_line, line_export);
		if (next_line == (char *)-1)
		{
			free(line_export);
			fclose(fd);
			release_exports(exports);
			return line_number;
		}

		/* must be called before setting uid/gid to default values
		because those are to be checked */
		if (validate_export(line_export, *exports) != 0)
		{
			free(line_export);
			fclose(fd);
			release_exports(exports);
			return line_number;
		}
		
		if (line_export->export_uid == (uid_t)-1)
		{
			line_export->export_uid = worker_uid;
		}
		
		if (line_export->path == NULL 
		|| line_export->users == NULL
		|| add_to_list(exports, line_export) == NULL)
		{
			free(line_export);
		}
	}
	while (next_line != NULL);

	free(buffer);
	fclose(fd);
	
	return 0;
}

static void release_export(struct rfs_export *single_export)
{
	free(single_export->path);

	struct list *user_item = single_export->users;
	while (user_item != NULL)
	{
		struct user_rec *rec = (struct user_rec *)user_item->data;
		if (rec->username != NULL)
		{
			free(rec->username);
		}

		if (rec->network != NULL)
		{
			free(rec->network);
		}
		
		user_item = user_item->next;
	}

	destroy_list(&(single_export->users));
	single_export->users = NULL;
}

void release_exports(struct list **exports)
{
	struct list *single_export = *exports;
	while (single_export != NULL)
	{
		struct list *next = single_export->next;
		release_export(single_export->data);
		single_export = next;
	}
	destroy_list(exports);
}

const struct rfs_export* get_export(const struct list *exports, const char *path)
{
	const struct list *single_export = exports;
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

#ifdef RFS_DEBUG
static void dump_export(const struct rfs_export *single_export)
{
	DEBUG("%s", "dumping export:\n");
	DEBUG("path: '%s'\n", single_export->path);

	struct list *user = single_export->users;
	DEBUG("%s\n", "users: ");
	while (user != NULL)
	{
		const struct user_rec *rec = (const struct user_rec *)user->data;
		
		DEBUG("'%s@%s/%u'\n", 
		rec->username != NULL ? rec->username : "", 
		rec->network != NULL ? rec->network : "", 
		rec->prefix_len);
		
		user = user->next;
	}

	DEBUG("options: %d\n", single_export->options);
	DEBUG("export uid: %d\n", single_export->export_uid);
}

void dump_exports(const struct list *exports)
{
	DEBUG("%s", "dumping exports set:\n");
	const struct list *single_export = exports;
	while (single_export != NULL)
	{
		dump_export(single_export->data);
		single_export = single_export->next;
	}
}
#endif /* RFS_DEBUG */
