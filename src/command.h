/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef COMMAND_H
#define COMMAND_H

/** rfs commands */

#include <stdint.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum server_commands
{ 
	cmd_first = 0, 
	
	/* rfs internal commands */
	cmd_auth               = 10,
	cmd_request_salt,
	cmd_closeconnection,
	cmd_changepath,
	cmd_keepalive,
	cmd_getexportopts,
	
	/* reserved */

	/* fs commands */
	cmd_readdir            = 50,
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
	cmd_chmod, 
	cmd_chown, 

	cmd_last
};

/** command for server */
struct command
{
	uint32_t command;
	uint32_t data_len;
};

/** answer to client */
struct answer
{
	uint32_t command;
	uint32_t data_len;
	int32_t ret;
	int16_t ret_errno;
};

/** get description of command. debug only */
const char* describe_command(const enum server_commands cmd);

/** write command to output. debug only */
void dump_command(const struct command *cmd);

/** write answer to output. debug only */
void dump_answer(const struct answer *cmd);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* COMMAND_H */
