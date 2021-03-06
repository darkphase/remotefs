/*
remotefs file system
See the file AUTHORS for copyright information.

This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <sys/types.h>

#include "config.h"
#include "changelog.h"
#include "version.h"

/* compatibility scheme is the following:
each number represents version where compatibility was broken

COMPAT_VERSION(0, 16) says that compatibility was broken in version 0.16
and versions 0.1-0.15 are compatible */

static int incompat_list[] =
{
	COMPAT_VERSION(0, 5), /* for testing purposes.
	0.9-0.15 don't have this feature, so incompatible by default */


	COMPAT_VERSION(0, 16), /* truncate modified to send offset as uint64_t instead of uint32_t,
	64-bit values in statfs() */

	COMPAT_VERSION(1, 1), /* stat modified to send inode number */
};

int versions_compatible(unsigned my_version, unsigned their_version)
{
	size_t changes = sizeof(incompat_list) / sizeof(incompat_list[0]);

	DEBUG("checking %d against %d\n", my_version, their_version);

	int max_version = (my_version > their_version ? my_version : their_version);
	int min_version = (max_version == my_version ? their_version : my_version);

	DEBUG("max version: %d, min version: %d\n", max_version, min_version);

	size_t i = 0; for (i = 0; i < changes; ++i)
	{
		if (incompat_list[i] > max_version)
		{
			return 1; /* don't look beyond max_version */
		}

		if (min_version < incompat_list[i])
		{
			return 0;
		}
	}

	return 1;
}

int my_version_compatible(unsigned their_version)
{
	return versions_compatible(
		COMPAT_VERSION(RFS_VERSION_MAJOR, RFS_VERSION_MINOR),
		their_version);
}
