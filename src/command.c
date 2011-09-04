/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <string.h>

#include "command.h"
#include "config.h"

const char* describe_command(const uint32_t cmd)
{
	switch (cmd)
	{
	/* rfs internal commands */
	case cmd_auth:                          return "auth";
	case cmd_request_salt:                  return "request salt";
	case cmd_closeconnection:               return "closeconnection";
	case cmd_changepath:                    return "changepath";
	case cmd_keepalive:                     return "keepalive";
	case cmd_getexportopts:                 return "getexportopts";
	case cmd_listexports:                   return "listexports";
	case cmd_handshake:                     return "handshake";
	
	/* fs commands */
	case cmd_readdir:                       return "readdir";
	case cmd_getattr:                       return "getattr";
	case cmd_mknod:                         return "mknod";
	case cmd_create:                        return "create";
	case cmd_open:                          return "open";
	case cmd_release:                       return "release";
	case cmd_truncate:                      return "truncate";
	case cmd_read:                          return "read";
	case cmd_write:                         return "write";
	case cmd_mkdir:                         return "mkdir";
	case cmd_unlink:                        return "unlink";
	case cmd_rmdir:                         return "rmdir";
	case cmd_rename:                        return "rename";
	case cmd_utime:                         return "utime";
	case cmd_utimens:                       return "utimens";
	case cmd_statfs:                        return "statfs";
	case cmd_chmod:                         return "chmod";
	case cmd_chown:                         return "chown";
	case cmd_lock:                          return "lock";
	case cmd_symlink:                       return "symlink";
	case cmd_link:                          return "link";
	case cmd_readlink:                      return "readlink";
	
	/* ACL */
	case cmd_setxattr:                      return "setxattr";
	case cmd_getxattr:                      return "getxattr";

	/* nss */
	case cmd_getnames:						return "getnames";
	case cmd_checkuser:						return "checkuser";
	case cmd_checkgroup:					return "checkgroup";
	case cmd_getusers:                      return "getusers";
	case cmd_getgroups:                     return "getgroups";

	case cmd_first:                         return "first command in list. should be unused. if you see this line in the output, then we have a problem";
	case cmd_last:                          return "last command in list. should be unused. if you see this line in the output, then we have another problem";
	default:                                return "no description yet";
	}
}

#ifdef RFS_DEBUG
void dump_command(const struct command *cmd)
{
	DEBUG("command: %s (%d), data length: %u\n", describe_command(cmd->command), cmd->command, cmd->data_len);
}

void dump_answer(const struct answer *ans)
{
	DEBUG("answer: %s (%d), data length: %u, ret: %d, errno: %s (%d)\n", describe_command(ans->command), ans->command, ans->data_len, ans->ret, strerror(ans->ret_errno), ans->ret_errno);
}
#endif
