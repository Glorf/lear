//
// Created by mbien on 08.11.18.
//

#ifndef PUTHTTPD_CONNECTION_H
#define PUTHTTPD_CONNECTION_H

typedef enum {
    UNINITIALIZED,
    FAILURE,
    RUNNING,
    DOWN
} e_server_status;

typedef struct {
    int srv_socket;
    e_server_status status;
} s_tcp_server;


int accept_client_connection(s_tcp_server *srv_in, int epoll_fd);
int read_client_connection(int cli_socket);
int bind_server_socket(unsigned short port, s_tcp_server *srv_out);
void create_server_struct(s_tcp_server *srv_out);
int make_socket_nonblocking(int fd);
int close_client_connection(int cli_socket);
int close_server_socket(s_tcp_server *srv_in);
void safe_write(int socket, char *data, unsigned long size);

#endif //PUTHTTPD_CONNECTION_H