/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include "options.h"

#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "buffer.h"
#include "command.h"
#include "config.h"
#include "instance_server.h"
#include "measure.h"
#include "memcache.h"
#include "sendrecv_server.h"
#include "server.h"
#include "sendfile/sendfile_rfs.h"

typedef int (*read_method)(struct rfsd_instance *instance, const struct command *cmd, uint64_t handle, off_t offset, size_t size);

static int read_small_block(struct rfsd_instance *instance, const struct command *cmd, uint64_t handle, off_t offset, size_t size)
{
	DEBUG("%s\n", "reading small block");

	if (size > SENDFILE_LIMIT)
	{
		return reject_request(instance, cmd, EOVERFLOW) == 0 ? 1 : -1;
	}
	
	int fd = (int)handle;
	
	char buffer[SENDFILE_LIMIT] = { 0 };

	errno = 0;
	ssize_t result = pread(fd, buffer, size, offset);
	
	struct answer ans = { cmd_read, (uint32_t)result, (int32_t)result, errno };

	if (result < 0)
	{
		return reject_request(instance, cmd, errno) == 0 ? 1 : -1;
	}

	send_token_t token = { 2, {{ 0 }} };
	token.iov[0].iov_base = (void *)hton_ans(&ans);
	token.iov[0].iov_len = sizeof(ans);
	token.iov[1].iov_base = (void *)buffer;
	token.iov[1].iov_len = (result >= 0 ? result : 0);

	return do_send(&instance->sendrecv, &token) < 0 ? -1 : 0;
}

#if (defined WITH_SSL || (! defined SENDFILE_AVAILABLE)) /* we don't need this on Linux/Solaris/FreeBSD/Darwin if SSL isn't enabled */
static int read_as_always(struct rfsd_instance *instance, const struct command *cmd, uint64_t handle, off_t offset, size_t size)
{
	DEBUG("%s\n", "reading as always");
	
	struct answer ans = { cmd_read, (uint32_t)size, (int32_t)size, 0 };
	char buffer[RFS_READ_BLOCK] = { 0 };
	int fd = (int)handle;	
	
	int first_block = 1;
	size_t done = 0;
	while (done < size)
	{
		unsigned current_block = ((size - done) >= RFS_READ_BLOCK ? RFS_READ_BLOCK : (size - done));
		
		errno = 0;
		ssize_t result = pread(fd, buffer, current_block, offset + done);
	
		if (result <= 0)
		{
			struct answer ans_error = { cmd_read, 0, done, errno };
			return (first_block ? rfs_send_answer : rfs_send_answer_oob)(&instance->sendrecv, &ans_error) == -1 ? -1 : 1;
		}
		else
		{
			send_token_t token = { 0, {{ 0 }} };
			if (do_send(&instance->sendrecv, 
				queue_data(buffer, result >= 0 ? result : 0, 
				first_block != 0 ? queue_ans(&ans, &token) : &token)) < 0)
			{
				return -1;
			}
			
			first_block = 0;
		}
		
		done += result;
	}

	return 0;
}
#endif /* defined WITH_SSL || ! defined SENDFILE_AVAILABLE */

#if defined SENDFILE_AVAILABLE
static inline read_method choose_read_method(struct rfsd_instance *instance, size_t read_size)
{
#ifdef WITH_SSL
	if (read_size <= SENDFILE_LIMIT)
	{
		return read_small_block;
	}

	return (instance->sendrecv.ssl_enabled != 0 ? read_as_always : read_with_sendfile);
#else
	return (read_size <= SENDFILE_LIMIT ? read_small_block : read_with_sendfile);
#endif /* SENDFILE_AVAILABLE */
}
#else /* sendfile isn't available */
static inline read_method choose_read_method(struct rfsd_instance *instance, size_t read_size)
{
	return (read_size <= SENDFILE_LIMIT ? read_small_block : read_as_always);
}
#endif /* defined SENDFILE_AVAILABLE */

int _handle_read(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd)
{
#define overall_size sizeof(handle) + sizeof(offset) + sizeof(size)
	uint64_t handle = (uint64_t)-1;
	uint64_t offset = 0;
	uint32_t size = 0;
	
	char read_buffer[overall_size] = { 0 };

	/* get parameters for the read call */
	if (rfs_receive_data(&instance->sendrecv, read_buffer, cmd->data_len) == -1)
	{
		return -1;
	}

	if (cmd->data_len != overall_size)
	{
		return reject_request(instance, cmd, EINVAL) == 0 ? 1 : -1;
	}
#undef overall_size
	
	unpack_64(&handle, 
	unpack_64(&offset, 
	unpack_32(&size, read_buffer
	)));

	DEBUG("handle: %llu, offset: %llu, size: %u\n", (unsigned long long)handle, (unsigned long long)offset, size);
	
	if (handle == (uint64_t)-1)
	{
		return reject_request(instance, cmd, EBADF) == 0 ? 1 : -1;
	}
	
#ifdef WITH_MEMCACHE
	if (size >= MEMCACHE_THRESHOLD 
	&& memcache_fits(&instance->memcache, handle, offset, size) != 0)
	{
		DEBUG("%s\n", "hooray! memcache fits!");
		
		struct memcached_block *cache = &instance->memcache;

		struct answer ans = { cmd_read, (uint32_t)cache->size, (int32_t)cache->size, 0 };

		send_token_t token = { 0, {{ 0 }} };

		if (do_send(&instance->sendrecv, 
			queue_data(cache->data, cache->size, 
			queue_ans(&ans, &token))) < 0)
		{
			return -1;
		}
	}
	else 
	{
#endif
		int ret = choose_read_method(instance, (size_t)size)(instance, cmd, handle, (off_t)offset, (size_t)size);

		if (ret != 0)
		{
			return ret;
		}
#ifdef WITH_MEMCACHE
	} /* else */

	/* try to read-ahead 
	we will ignore any errors at this point since real read operation is already complete */
	if (size >= MEMCACHE_THRESHOLD)
	{
		DEBUG("memcaching block at %llu of size %llu (desc %llu)\n", 
		(long long unsigned)offset + size, 
		(long long unsigned)size, 
		(long long unsigned)handle);

		struct memcached_block *cache = &instance->memcache;

		if (cache->size != size)
		{
			if (cache->data != NULL)
			{
				free(cache->data);
				cache->data = NULL;
			}

			cache->data = malloc(size);
			DEBUG("reserved cache data at %p\n", cache->data);
			if (cache->data == NULL)
			{
				destroy_memcache(cache);
				return 0;
			}
		}

		ssize_t result = pread((int)handle, cache->data, size, offset + size);
		if (result != size)
		{
			destroy_memcache(cache);
		}
		else
		{
			memcache_block(cache, handle, offset + size, size, NULL);
		}
	}
#endif

	return 0;
}

