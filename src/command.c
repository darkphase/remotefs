#include "command.h"

#include <string.h>

#include "config.h"

const char* describe_command(const enum server_commands cmd)
{
	switch (cmd)
	{
	case cmd_auth:				return "auth";
	case cmd_closeconnection: 	return "closeconnection";
	case cmd_changepath: 		return "changepath";
	case cmd_keepalive: 		return "keepalive";
	
	case cmd_readdir:			return "readdir";
	
	case cmd_getattr:			return "getattr";
	
	case cmd_mknod:				return "mknod";
	case cmd_open:				return "open";
	case cmd_release:			return "release";
	case cmd_truncate: 			return "truncate";
	
	case cmd_read:				return "read";
	case cmd_write:				return "write";
	
	case cmd_mkdir:				return "mkdir";
	case cmd_unlink:			return "unlink";
	case cmd_rmdir:				return "rmdir";
	case cmd_rename:			return "rename";
	
	case cmd_utime:				return "utime";
	case cmd_statfs:			return "statfs";	
	
	case cmd_first: 			return "first command in list. should be unused. if you see this line in the output, then we have a problem";
	case cmd_last:				return "last command in list. should be unused. if you see this line in the output, then we have another problem";
	default:					return "no description yet";
	}
}

void dump_command(const struct command *cmd)
{
	DEBUG("command: %s (%d), data length: %u\n", describe_command(cmd->command), cmd->command, cmd->data_len);
}

void dump_answer(const struct answer *ans)
{
	DEBUG("answer: %s (%d), data length: %u, ret: %d, errno: %s (%d)\n", describe_command(ans->command), ans->command, ans->data_len, ans->ret, strerror(ans->ret_errno), ans->ret_errno);
}
