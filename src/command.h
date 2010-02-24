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

/** commands for remotefs server */
enum server_commands
{ 
	cmd_first = 0, 
	
	/* rfs internal commands */
	cmd_auth               = 10, /* 0a */
	cmd_request_salt,            /* 0b */
	cmd_closeconnection,         /* 0c */
	cmd_changepath,              /* 0d */
	cmd_keepalive,               /* 0e */
	cmd_getexportopts,           /* 0f */
	cmd_enablessl,               /* 12 */
	cmd_listexports,             /* 13 */
	cmd_getnames,                /* 14 */
	
	/* reserved */

	/* fs commands */
	cmd_readdir            = 50, /* 32 */
	cmd_getattr,                 /* 33 */
	cmd_mknod,                   /* 34 */
	cmd_open,                    /* 35 */
	cmd_release,                 /* 36 */
	cmd_truncate,                /* 37 */
	cmd_read,                    /* 38 */
	cmd_write,                   /* 39 */
	cmd_mkdir,                   /* 3A */
	cmd_unlink,                  /* 3B */
	cmd_rmdir,                   /* 3C */
	cmd_rename,                  /* 3D */
	cmd_utime,                   /* 3E */
	cmd_statfs,                  /* 3F */
	cmd_chmod,                   /* 40 */
	cmd_chown,                   /* 41 */
	cmd_lock,                    /* 42 */
	cmd_symlink,                 /* 43 */
	cmd_link,                    /* 44 */
	cmd_readlink,                /* 45 */
	cmd_getxattr,                /* 46 */
	cmd_setxattr,                /* 47 */
	cmd_utimens,                 /* 48 */
	cmd_create,                  /* 49 */

	/* reserved */

	/* nss */
	cmd_checkuser          = 100, 
	cmd_checkgroup, 
	cmd_getusers, 
	cmd_getgroups, 

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
	int32_t ret_errno;
};

/** get description of command. debug only */
const char* describe_command(const uint32_t cmd);

#ifdef RFS_DEBUG
/** write command to output. debug only */
void dump_command(const struct command *cmd);

/** write answer to output. debug only */
void dump_answer(const struct answer *cmd);
#endif

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* COMMAND_H */

