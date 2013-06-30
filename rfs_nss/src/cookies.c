/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <search.h>
#include <stdlib.h>
#include <time.h>

#include "config.h"
#include "cookies.h"

static int compare_pids(const void *node1, const void *node2)
{
	pid_t pid1 = ((struct cookie *)node1)->pid;
	pid_t pid2 = ((struct cookie *)node2)->pid;

	if (pid1 < pid2) return -1;
	if (pid1 == pid2) return 0;
	return 1;
}

static int any_node(const void *node1, const void *node2)
{
	return 0;
}

static int compare_time(const void *s1, const void *s2)
{
	return ((struct cookie *)s1)->updated > (((struct cookie *)s2)->updated + COOKIES_TTL)
	? 0
	: 1;
}

struct cookie* create_cookie(void **cookies, pid_t pid)
{
	struct cookie *key = malloc(sizeof(*key));

	if (key == NULL)
	{
		return NULL;
	}

	key->pid = pid;
	key->value = 0;

	void *found = tsearch(key, cookies, compare_pids);
	(*(struct cookie **)found)->updated = time(NULL);

	return *(struct cookie **)found;
}

int delete_cookie(void **cookies, pid_t pid)
{
	struct cookie key = { pid, 0 };

	return (tdelete(&key, cookies, compare_pids) != NULL ? 0 : -1);
}

struct cookie* get_cookie(void **cookies, pid_t pid)
{	
	struct cookie key = { pid, 0 };

	void *found = tfind(&key, cookies, compare_pids);
	return (found == NULL ? NULL : *(struct cookie **)found);
}

void destroy_cookies(void **cookies)
{
	while (*cookies != NULL)
	{
		tdelete(NULL, cookies, any_node);
	}
	
	*cookies = NULL;
}

void clear_cookies(void **cookies)
{
	DEBUG("%s\n", "clearing cookies");

	struct cookie key = { 0 };

	key.updated = time(NULL);

	do
	{
		void *found = tfind(&key, cookies, compare_time);

		if (found == NULL)
		{
			break;
		}
		
		struct cookie *cookie = *(struct cookie **)found;

		if (tdelete(cookie, cookies, compare_pids) != NULL)
		{
			DEBUG("deleted cookie for pid %d\n", cookie->pid);
		}
	}
	while (1);
}

#ifdef RFS_DEBUG
void dump_cookie(const struct cookie *cookie)
{
	DEBUG("cookie pid: %d, value: %u\n", cookie->pid, cookie->value);
}

static void dump(const void *node, const VISIT which, const int depth)
{
	if (which == endorder
	|| which == leaf)
	{
		dump_cookie(*(struct cookie **)node);
	}
}

void dump_cookies(const void *cookies)
{
	twalk(cookies, dump);
}
#endif

