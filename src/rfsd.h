#ifndef SERVER_H
#define SERVER_H

void server_close_connection(int socket);
int add_file_to_open_list(int file);
int remove_file_from_open_list(int file);

#endif // SERVER_H
