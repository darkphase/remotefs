/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include "../options.h"

#if (defined LINUX && defined ACL_AVAILABLE)

#include <errno.h>
#include <string.h>
#include <sys/acl.h>

#include "acl_linux.h"
#include "acl_utils.h"
#include "libacl/include/acl_ea.h"
#include "../buffer.h"
#include "../config.h"
#include "../inet.h"

struct acl_size_params
{
	size_t count;
};

static int acl_size_callback(acl_tag_t tag, int perms, void *id, void *params_casted)
{
	++(((struct acl_size_params *)params_casted)->count);

	return 0;
}

size_t xattr_acl_size(const acl_t acl, size_t *recs_number)
{
	if (recs_number != NULL)
	{
		*recs_number = 0;
	}

	struct acl_size_params params = { 0 };
	int walk_ret = walk_acl(acl, acl_size_callback, (void *)&params);

	if (walk_ret != 0)
	{
		return 0;
	}
	
	if (recs_number != NULL)
	{
		*recs_number = params.count;
	}

	return acl_ea_size(params.count);
}

struct acl_to_xattr_params
{
	acl_ea_header *acl;
	size_t count;
};
	
static int acl_to_xattr_callback(acl_tag_t tag, int perms, void *id, void *params_casted)
{
	struct acl_to_xattr_params *params = (struct acl_to_xattr_params *)(params_casted);

	params->acl->a_entries[params->count].e_tag  = htole16((uint16_t)(tag));
	params->acl->a_entries[params->count].e_perm = htole16((uint16_t)(perms));
	params->acl->a_entries[params->count].e_id   = htole32((id == NULL ? ACL_UNDEFINED_ID : *(uint32_t *)(id)));

	++params->count;

	return 0;
}

int rfs_acl_to_xattr(const acl_t acl, void *xattr, size_t size)
{
	size_t entries_number = 0;
	size_t rec_size = xattr_acl_size(acl, &entries_number);

	if (rec_size > size)
	{
		return -ERANGE;
	}

	acl_ea_header *acl_rec = (acl_ea_header *)(xattr);
	acl_rec->a_version = htole32(ACL_EA_VERSION);

	struct acl_to_xattr_params params = { acl_rec, 0 };

	int walk_ret = walk_acl(acl, acl_to_xattr_callback, (void *)&params);
	if (walk_ret != 0)
	{
		return walk_ret;
	}


	return 0;
}

int rfs_acl_from_xattr(const void *xattr, size_t size, acl_t *acl)
{
	size_t entries_number = acl_ea_count(size);

	DEBUG("entries number: %lu\n", (unsigned long)entries_number);

	errno = 0;
	acl_t acl_ret = acl_init(entries_number);
	if (acl_ret == NULL)
	{
		return -errno;
	}

	const acl_ea_header *acl_rec = (acl_ea_header *)(xattr);

	size_t i = 0; for (; i < entries_number; ++i)
	{
		acl_entry_t entry = NULL;
		const acl_ea_entry *acl_rec_entry = &acl_rec->a_entries[i];

		errno = 0;
		if (acl_create_entry(&acl_ret, &entry) != 0)
		{
			goto error;
		}

		errno = 0;
		if (acl_set_tag_type(entry, acl_rec_entry->e_tag) != 0)
		{
			goto error;
		}

		switch (acl_rec_entry->e_tag)
		{
		case ACL_USER:
			{
			uid_t uid = (uid_t)(acl_rec_entry->e_id);
			errno = 0;
			if (acl_set_qualifier(entry, (void *)(&uid)) != 0)
			{
				goto error;
			}
			}
			break;
		case ACL_GROUP:
			{
			gid_t gid = (gid_t)(acl_rec_entry->e_id);
			errno = 0;
			if (acl_set_qualifier(entry, (void *)(&gid)) != 0)
			{
				goto error;
			}
			}
			break;
		}

		acl_permset_t permset = NULL;
		if (acl_get_permset(entry, &permset) != 0)
		{
			goto error;
		}

		if ((acl_rec_entry->e_perm & ACL_READ) != 0)
		{
			errno = 0;
			if (acl_add_perm(permset, ACL_READ) != 0)
			{
				goto error;
			}
		}

		if ((acl_rec_entry->e_perm & ACL_WRITE) != 0)
		{
			errno = 0;
			if (acl_add_perm(permset, ACL_WRITE) != 0)
			{
				goto error;
			}
		}

		if ((acl_rec_entry->e_perm & ACL_EXECUTE) != 0)
		{
			errno = 0;
			if (acl_add_perm(permset, ACL_EXECUTE) != 0)
			{
				goto error;
			}
		}

		errno = 0;
		if (acl_set_permset(entry, permset) != 0)
		{
			goto error;
		}
	}

	*acl = acl_ret;

	return 0;

error:
	{
	int saved_errno = errno;
	acl_free(acl);
	return -saved_errno;
	}
}

#else
int acl_linux_c_empty_module = 0;
#endif /* ACL_AVAILABLE && LINUX */
