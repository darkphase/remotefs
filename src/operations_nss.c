#include <dlfcn.h>
#include "rfs_nsslib.h"

static uid_t (*putpwnam)(const char*, char*);
static gid_t (*putgrnam)(const char*, char*);

int init_rfs_nss(void)
{
	void *dl_hdl;
	if ( putpwnam != NULL || putgrnam != NULL )
	{
		return 0;
	}

	if ( (dl_hdl=dlopen(LIBRFS_NSS, RTLD_LAZY)) == NULL )
	{
		return 1;
	}

	/* get the functions pointer*/
	if ( (putpwnam = (uid_t (*)(const char*, char*))dlsym(dl_hdl,"rfs_putpwnam")) == NULL )
	{
		return 1;
	}

	if ( (putgrnam = (gid_t (*)(const char*, char*))dlsym(dl_hdl, "rfs_putgrnam")) == NULL )
	{
		return 1;
	}
	return 0;
}

static void insert_all_name(char *buffer, char *host)
{
	int ret;
	/* do the hard job */
	while ( buffer && *buffer)
	{
		if ( strcmp("root",buffer) && strchr(buffer,'@') == NULL )
		{
			ret = putpwnam(buffer, host);
			if ( ret == 0 )
			{
				return;
			}
		}
		buffer += strlen(buffer) + 1;
	}

	buffer++;
	while ( buffer  && *buffer)
	{
		if ( strcmp("root",buffer) && strchr(buffer,'@') == NULL )
		{
			ret = putgrnam(buffer, host);
			if ( ret == 0 )
			{
				return;
			}
		}
		buffer += strlen(buffer) + 1;
	}
}

static inline int check_names(char *buffer, int size)
{
	char *tmp = buffer;
	if ( size < 2 || buffer[size-2] || buffer[size-1] )
	{
		return -1;
	}

	while(*tmp)
	{
		while(*tmp) tmp++;
		tmp++;
	}
	if (tmp-buffer >= size )
	{
		return -1;
	}

	tmp++;
	while(*tmp)
	{
		while(*tmp) tmp++;
		tmp++;
	}
	if (tmp-buffer > size )
	{
		return -1;
	}
	return 0;
}

int rfs_nss(struct rfs_instance *instance)
{
	if (instance->sendrecv.socket == -1)
	{
		return -ECONNABORTED;
	}

	struct command cmd = { cmd_nss, 0 };
	if (rfs_send_cmd(&instance->sendrecv, &cmd) == -1)
	{
		return -ECONNABORTED;
	}

	struct answer ans = { 0 };

	if (rfs_receive_answer(&instance->sendrecv, &ans) == -1)
	{
		return -ECONNABORTED;
	}

	if (ans.command != cmd_nss)
	{
		return cleanup_badmsg(instance, &ans);
	}

	char *buffer = get_buffer(ans.data_len);
	memset(buffer, 0, ans.data_len);

	if (rfs_receive_data(&instance->sendrecv, buffer, ans.data_len) == -1)
	{
		free_buffer(buffer);
		return -ECONNABORTED;
	}
	

	/* check if data are build as expected */
	if ( check_names(buffer, ans.data_len) == -1 )
	{
		free_buffer(buffer);
		return -ECONNABORTED;
	}

	/* put name to rfs_nss */
	insert_all_name(buffer, instance->config.host);

	free_buffer(buffer);
	return 0;

}
