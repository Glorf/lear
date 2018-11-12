//
// Created by mbien on 08.11.18.
//

#ifndef PUTHTTPD_CONNECTION_H
#define PUTHTTPD_CONNECTION_H

enum server_status {
    UNINITIALIZED,
    FAILURE,
    RUNNING,
    DOWN
};
typedef enum server_status e_server_status;

struct tcp_server {
    int srv_socket;
    e_server_status status;
};
typedef struct tcp_server s_tcp_server;

struct tcp_client {
    int cli_socket;
    s_tcp_server *conn_srv;
};
typedef struct tcp_client s_tcp_client;

int accept_client_connection(s_tcp_server *srv_in, s_tcp_client *cli_out);
int listen_client_connection(s_tcp_server *srv_in);
int bind_server_socket(unsigned short port, s_tcp_server *srv_out);

#endif //PUTHTTPD_CONNECTION_H