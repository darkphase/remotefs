#ifndef COMMAND_H
#define COMMAND_H

#include <stdint.h>

enum server_commands
{ 
	cmd_first = 0, 
	
	cmd_auth,
	cmd_request_salt,
	cmd_closeconnection,
	cmd_changepath,
	cmd_keepalive,

	cmd_readdir,
	
	cmd_getattr,
	
	cmd_mknod,
	cmd_open,
	cmd_release,
	cmd_truncate,
	
	cmd_read,
	cmd_write,
	
	cmd_mkdir,
	cmd_unlink,
	cmd_rmdir,
	cmd_rename,
	
	cmd_utime,
	cmd_statfs,
	
	cmd_last,
};

struct command
{
	uint32_t command;
	uint32_t data_len;
};

struct answer
{
	uint32_t command;
	uint32_t data_len;
	int32_t ret;
	int16_t ret_errno;
};

const char* describe_command(const enum server_commands cmd);

void dump_command(const struct command *cmd);
void dump_answer(const struct answer *cmd);

#endif // COMMAND_H
