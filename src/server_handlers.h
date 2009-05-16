/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef SERVER_HANDLERS_H
#define SERVER_HANDLERS_H

/** server handlers of rfs operations */

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct command;
struct rfsd_instance;
struct sockaddr_in;

int _handle_request_salt(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int _handle_auth(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int _handle_closeconnection(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int _handle_changepath(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int _handle_keepalive(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int _handle_getexportopts(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
#ifdef WITH_SSL
int _handle_enablessl(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
#endif

#ifdef WITH_EXPORTS_LIST
int _handle_listexports(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
#endif

int _handle_getattr(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int _handle_readdir(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int _handle_mknod(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int _handle_open(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int _handle_read(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int _handle_write(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int _handle_truncate(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int _handle_mkdir(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int _handle_unlink(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int _handle_rmdir(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int _handle_rename(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int _handle_utime(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int _handle_statfs(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int _handle_release(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int _handle_chmod(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int _handle_chown(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int _handle_lock(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int _handle_link(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int _handle_symlink(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int _handle_readlink(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
#if defined WITH_ACL
int _handle_getxattr(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int _handle_setxattr(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
#endif

int _handle_getnames(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* SERVER_HANDLERS_H */

