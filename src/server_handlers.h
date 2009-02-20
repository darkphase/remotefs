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

struct sockaddr_in;
struct command;
struct rfsd_instance;

int handle_request_salt(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_auth(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_closeconnection(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_changepath(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_keepalive(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_getexportopts(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_setsocktimeout(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_setsockbuffer(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
#ifdef WITH_SSL
int handle_enablessl(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
#endif
#ifdef WITH_EXPORTS_LIST
int handle_listexports(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
#endif

int handle_getattr(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_readdir(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_mknod(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_open(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_read(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_write(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_truncate(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_mkdir(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_unlink(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_rmdir(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_rename(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_utime(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_statfs(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_release(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_chmod(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_chown(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_lock(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
#if defined WITH_LINKS
int handle_link(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_symlink(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_readlink(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
#endif
#if defined WITH_ACL
int handle_getxattr(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_setxattr(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
#endif
#if defined WITH_NSS
int handle_nss(struct rfsd_instance *instance, const struct sockaddr_in *client_addr, const struct command *cmd);
#endif
#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* SERVER_HANDLERS_H */
