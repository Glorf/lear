//
// Created by mbien on 08.11.18.
//

#ifndef PUTHTTPD_CONNECTION_H
#define PUTHTTPD_CONNECTION_H

#include "http.h"

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

typedef struct s_connection s_connection;

struct s_connection {
    int fd;
    long drop_timeout; //time when timeout should be triggered
    s_buffer request_buffer;
    s_http_request *currentRequest;
    int requestQueue;
    s_buffer response_buffer;
    s_connection *next;
    s_connection *prev;
};

int accept_client_connection(s_tcp_server *srv_in, int epoll_fd);
long read_client_connection(s_connection *cli_socket);
long write_client_connection(s_connection *cli_socket);
int process_client_connection(s_connection *cli_socket);
int bind_server_socket(unsigned short port, s_tcp_server *srv_out);
void create_server_struct(s_tcp_server *srv_out);
int make_socket_nonblocking(int fd);
int close_client_connection(s_connection *cli_socket);
void detach_client_connection(s_connection *cli_socket);
int close_server_socket(s_tcp_server *srv_in);

#endif //PUTHTTPD_CONNECTION_H