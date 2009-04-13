/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifdef WITH_ACL

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/acl.h>

#include "acl_utils.h"
#include "acl_utils_nss.h"
#include "config.h"
#include "id_lookup.h"
#include "instance_client.h"

unsigned acl_need_nss_patching(const char *acl_text)
{
	return strchr(acl_text, '@') != NULL;
}

char* patch_acl_for_server(const char *acl_text, const struct rfs_instance *instance)
{
	const char *start = acl_text;
	const char *delim = NULL;
	size_t leftovers = 0;
	size_t host_len = strlen(instance->config.host);

	/* calc leftovers after patching and check server name */
	while ((delim = strchr(start, '@')) != NULL)
	{
		const char *server_end = strchr(delim + 1, ':');
		if (server_end == NULL)
		{
			server_end = delim + 1 + strlen(delim + 1);
		}

		size_t server_len = server_end - (delim + 1);

		if (server_len != host_len)
		{
			return NULL;
		}

		if (strncmp(instance->config.host, delim + 1, server_len) != 0)
		{
			return NULL;
		}

		leftovers += host_len + 1; /* +1 == strlen("@") */

		start = delim + 1;
	}

	size_t acl_len = strlen(acl_text);
	size_t patched_size = strlen(acl_text) - leftovers + 1;
	size_t done = 0;

	char *patched = malloc(patched_size);

	/* patch acl */
	start = acl_text;
	while ((delim = strchr(start, '@')) != NULL)
	{
		size_t text_len = delim - start;

		if (done + text_len >= patched_size)
		{
			free(patched);
			return NULL;
		}

		strncpy(patched + done, start, delim - start);

		done += text_len;
		start = delim + host_len + 1;
	}

	if (start < acl_text + acl_len)
	{
		size_t last_len = strlen(start);

		DEBUG("done: %ld, left: %ld, expected: %ld\n", done, last_len, patched_size);

		if (done + last_len >= patched_size)
		{
			free(patched);
			return NULL;
		}

		strncpy(patched + done, start, last_len);
		done += last_len;
	}
	
	patched[done] = 0;

	DEBUG("done patched: %ld, expected: %ld\n", done, patched_size - 1);

	if (done < patched_size - 1)
	{
		free(patched);
		return NULL;
	}

	return patched;
}

int patch_acl_from_server(rfs_acl_t *acl, int count, const struct rfs_instance *instance)
{
	size_t host_len = strlen(instance->config.host);

	int i = 0; for (i = 0; i < count; ++i)
	{
		const char *name = NULL;

		if (acl->a_entries[i].e_id == -1)
		{
			continue;
		}
		
		DEBUG("acl id: %d\n", acl->a_entries[i].e_id);

		switch (acl->a_entries[i].e_tag)
		{
		case ACL_USER:
			name = get_uid_name(instance->id_lookup.uids, (uid_t)acl->a_entries[i].e_id);
			break;
		case ACL_GROUP:	
			name = get_gid_name(instance->id_lookup.gids, (gid_t)acl->a_entries[i].e_id);
			break;
		}

		DEBUG("name: %s\n", name);

		/* try too lookup the same name, but with @host */

		size_t full_len = strlen(name) + 1 + host_len + 1;
		char *full_name = malloc(full_len);

		if (snprintf(full_name, full_len, "%s@%s", name, instance->config.host) >= full_len)
		{
			free(full_name);
			return -EINVAL;
		}

		switch (acl->a_entries[i].e_tag)
		{
		case ACL_USER:
			{
			uid_t uid = get_uid(instance->id_lookup.uids, full_name);
			if (uid != (uid_t)-1)
			{
				acl->a_entries[i].e_id = (long int)uid;
			}
			}
			break;
		case ACL_GROUP:	
			{
			gid_t gid = get_gid(instance->id_lookup.gids, full_name);
			if (gid != (gid_t)-1)
			{
				acl->a_entries[i].e_id = (long int)gid;
			}
			}
			break;
		}

		free(full_name);
	}

	return 0;
}

#else
int acl_utils_nss_c_empty_module_makes_suncc_mad = 0;
#endif /* WITH_ACL*/


