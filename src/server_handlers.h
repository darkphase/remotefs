#ifndef SERVER_HANDLERS_H
#define SERVER_HANDLERS_H

struct sockaddr_in;
struct command;

#define SERVER_VERSION 1

int handle_auth(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_closeconnection(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_changepath(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_keepalive(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd);

int handle_getattr(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_readdir(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_mknod(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_open(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_read(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_write(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_truncate(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_mkdir(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_unlink(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_rmdir(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_rename(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_utime(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_statfs(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd);
int handle_release(const int client_socket, const struct sockaddr_in *client_addr, const struct command *cmd);

#endif // SERVER_HANDLERS_H
