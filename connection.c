#include "connection.h"
#include "config.h"
#include "logger.h"

#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

int accept_client_connection(s_tcp_server *srv_in, s_tcp_client *cli_out) {
    if(srv_in->status != RUNNING) {
        message_log("Failed to assign connection to server in non-running state", ERR);
        return -1;
    }
    cli_out->cli_socket = accept(srv_in->srv_socket, NULL, NULL);
    if(cli_out->cli_socket < 0) {
        message_log("Failed to initialize connection to new client", ERR);
        srv_in->status = FAILURE;
        return -1;
    }
    cli_out->conn_srv = srv_in;

    return 0;
}

int close_client_connection(s_tcp_client *cli_in) {
    int res = close(cli_in->cli_socket);
    if(res < 0) {
        message_log("Failed to close connection from to client", ERR);
        cli_in->conn_srv->status = FAILURE;
        return -1;
    }

    return 0;
}

int bind_server_socket(unsigned short port, s_tcp_server *srv_out) {
    srv_out->status = UNINITIALIZED;

    //Bind socket
    srv_out->srv_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(srv_out->srv_socket < 0) {
        message_log("Failed to initialize server socket", ERR);
        return -1;
    }

    //Set socket properties
    char opt = 1;
    setsockopt(srv_out->srv_socket, SOL_SOCKET, SO_REUSEADDR, &opt,  sizeof(opt));

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(struct sockaddr));

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(port);

    int err;
    err = bind(srv_out->srv_socket, (struct sockaddr*)&server_address, sizeof(struct sockaddr));
    if(err < 0) {
        message_log("Bind attempt failed", ERR);
        return -1;
    }

    err = listen(srv_out->srv_socket, read_config_int("queueSize", "5"));
    if(err < 0) {
        message_log("Error while listening on port", ERR);
        return -1;
    }

    srv_out->status = RUNNING;

    return 0;
}

int close_server_socket(s_tcp_server *srv_in) {
    int res = close(srv_in->srv_socket);
    if(res < 0) {
        message_log("Failed to close server socket", ERR);
        srv_in->status = FAILURE;
        return -1;
    }

    srv_in->status = DOWN;

    return 0;
}