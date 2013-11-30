/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "../buffer.h"
#include "../command.h"
#include "../config.h"
#include "../instance_client.h"
#include "../list.h"
#include "../sendrecv_client.h"
#include "utils.h"

static unsigned count_components(const char *path)
{
	unsigned components = 0;
	const char *p = path;

	while (*p != 0 
	&& *p == '/')
	{
		++p;
	}

	size_t path_len = strlen(p);
	size_t i = 0; for (i = 0; i < path_len; ++i)
	{
		if (p[i] == '/')
		{
			++components;
		}
	}

	return components;
}

static const char* common_tail(const char *mounted_path, const char *link)
{
	size_t path_len = strlen(mounted_path);

	if (path_len > strlen(link)
	|| memcmp(mounted_path, link, path_len) != 0)
	{
		return NULL;
	}

	const char *tail = link + path_len;

	while (*tail != 0 
	&& *tail == '/')
	{
		++tail;
	}

	return tail;
}

char* _transform_symlink(struct rfs_instance *instance, const char *path, const char *link)
{
	DEBUG("transforming symlink with path %s (%s)\n", path, link);

	if (link[0] != '/')
	{
		return NULL;
	}

	const char *tail = common_tail(instance->config.path, link);
	if (tail == NULL)
	{
		return NULL;
	}
	
	size_t tail_len = strlen(tail);

	if (tail_len == 0)
	{
		char *dot = malloc(2);
		dot[0] = '.';
		dot[1] = 0;
		return dot;
	}

	DEBUG("link tail: %s\n", tail);

	unsigned components = count_components(path);

	DEBUG("components: %u\n", components);

	if (components == 0)
	{
		char *tail_copy = malloc(tail_len + 1);
		memcpy(tail_copy, tail, tail_len);
		tail_copy[tail_len] = 0;
		return tail_copy;
	}

	const char *prepend = "../";
	size_t prepend_len = strlen(prepend);

	unsigned tail_pos = components * prepend_len;
	unsigned need_memory = tail_pos + tail_len + 1;

	char *new_link = malloc(need_memory);
	unsigned i = 0; for (i = 0; i < components; ++i)
	{
		memcpy(new_link + prepend_len * i, prepend, prepend_len);
	}

	DEBUG("tail pos: %u\n", tail_pos);

	memcpy(new_link + tail_pos, tail, tail_len);
	new_link[need_memory - 1] = 0;

	DEBUG("new link: %s\n", new_link);

	return new_link;
}

int _rfs_readlink(struct rfs_instance *instance, const char *path, char *link_buffer, size_t size)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	unsigned path_len = strlen(path) + 1;
	uint32_t bsize = size - 1; /* reserve place for ending \0 */

	int overall_size = path_len + sizeof(bsize);
	
	struct rfs_command cmd = { cmd_readlink, overall_size };

	char *buffer = malloc(overall_size);

	pack(path, path_len, 
	pack_32(&bsize, buffer
	));

	send_token_t token = { 0 };
	if (do_send(&instance->sendrecv, 
		queue_data(buffer, overall_size, 
		queue_cmd(&cmd, &token))) < 0)
	{
		free(buffer);
		return -ECONNABORTED;
	}

	free(buffer);

	struct rfs_answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}

	if (ans.command != cmd_readlink
	|| ans.data_len > bsize)
	{
		return cleanup_badmsg(instance, &ans);
	}

	/* if all was OK we will get the link info within our telegram */
	if (ans.ret != 0)
	{
		return -ans.ret_errno;
	}

	buffer = malloc(ans.data_len);
	memset(link_buffer, 0, ans.data_len);

	if (rfs_receive_data(&instance->sendrecv, buffer, ans.data_len) == -1)
	{
		free(buffer);
		return -ECONNABORTED;
	}
	
	if (ans.data_len >= size) /* >= to fit ending \0 into link_buffer */
	{
		free(buffer);
		return -EBADMSG;
	}
			
	char *link = buffer;
	size_t link_len = strlen(buffer);
		
	if (instance->config.transform_symlinks != 0)
	{
		char *transformed_link = _transform_symlink(instance, path, link);

		if (transformed_link != NULL)
		{
			size_t transformed_link_len = strlen(transformed_link);

			if (transformed_link_len >= size)
			{
				free(transformed_link);
			}
			else
			{
				free(buffer);
				link_len = strlen(transformed_link);
				link = transformed_link;
			}
		}
	}

	strncpy(link_buffer, link, link_len);
	link_buffer[link_len] = 0;
	free(link);
	
	return 0;
}
