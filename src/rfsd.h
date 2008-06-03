#ifndef SERVER_H
#define SERVER_H

int add_file_to_open_list(int file);
int remove_file_from_open_list(int file);
void server_close_connection(int socket);
void stop_server();
void check_keep_alive();

#endif // SERVER_H
