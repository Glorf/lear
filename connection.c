#include "connection.h"
#include "config.h"
#include "logger.h"
#include "http.h"

#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

int accept_client_connection(s_tcp_server *srv_in, int epoll_fd) {
    if(srv_in->status != RUNNING) {
        message_log("Failed to assign connection to server in non-running state", ERR);
        return -1;
    }
    int cli_socket = accept(srv_in->srv_socket, NULL, NULL);
    if(cli_socket < 0) {
        if(errno == EAGAIN || errno == EWOULDBLOCK) {
            message_log("Failed to initialize connection to any new client", DEBUG);
            return -1;
        }
        else {
            message_log("Failed to accept incoming connection", ERR);
            return -1;
        }
    }

    if(make_socket_nonblocking(cli_socket) < 0) {
        message_log("Failed to make client connection nonblocking", ERR);
        return -1;
    }

    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = cli_socket;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cli_socket, &event) < 0) {
        message_log("Epoll error adding accepted connection", ERR);
        return -1;
    }
    message_log("New connection processed.", DEBUG);

    return 0;
}

int read_client_connection(int cli_socket) {
    message_log("Request processing started", INFO);
    s_http_request request;
    request.method = UNKNOWN;

    long timeout = read_config_long("requestTimeout", "5");
    time_t start = time(NULL);

    char line[read_config_long("maxRequestSize", "8192")];
    int linesize = 0;
    for(; ;) {
        ssize_t count;
        char buf;

        count = read(cli_socket, &buf, 1);
        if (count == -1) {
            if (errno != EAGAIN) {
                message_log("Error while reading from client", WARN);
                close(cli_socket);
                return -1;
            }

            return -1;
        } else if (count == 0) {
            if(time(NULL)-start>timeout) { //Wait n seconds for potential next request or next part of current request
                message_log("Request timeout", INFO);
                close(cli_socket);
                return -1;
            }
            else
                continue;
        }

        if(linesize > 0 && buf == 10 && line[linesize-1] == 13) { //CRLF
            if(linesize == 1) { //End of HTTP request
                message_log("Request finished", DEBUG);
                break;
            }

            int status = parse_request_line(line, linesize-1, &(request));  //Parse the finished request line (minus clrf)

            if(status != OK) {
                if(status == BAD_REQUEST) {
                    message_log("Bad request!!", WARN);
                }
                //message_log("Invalid request or unknown request parameter", WARN);
            }
            linesize = 0;
        }
        else {
            line[linesize] = buf;
            linesize++;
        }
    }

    s_http_response response;
    if(process_http_request(&request, &response) < 0) { //Main request processing thread
        message_log("Failed to produce response", ERR);
        /*
         * TODO: Should return 500
         */

        return -1;
    }


    s_string headerString = generate_bare_header(&response);

    if(headerString.length <= 0) {
        message_log("Failed to serialize server response", ERR);
        /*
         * TODO: Should return 500
         */
        return -1;
    }

    /*if(responseString.length > read_config_int("maxResponseSize", "81920000")) {
        message_log("Requested file is too big", ERR);
        //TODO: Should return 500

        return -1;
    }*/

    /* Write message header */
    safe_write(cli_socket, headerString.position, headerString.length);

    if(headerString.position == NULL)
        message_log("Something weird just happened!", ERR);
    else
        free(headerString.position);

    /* Write message body */
    if(response.body_length>0) safe_write(cli_socket, response.body, response.body_length);

    return 0;
}

void safe_write(int socket, char *data, unsigned long size) {
    long timeout = read_config_long("requestTimeout", "5");
    time_t start = time(NULL);

    size_t sent = 0;

    while(sent < size && time(NULL)-start < timeout) {
        ssize_t s = write(socket, data+sent, size-sent);
        if(s == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
            message_log("Error while writing to client", ERR);
            return;
        }
        else if(s > 0) {
            sent += s;
            start = time(NULL);
        }
    }

    if(sent < size) {
        message_log("Write timeout", ERR);
    }
}

int close_client_connection(int cli_socket) {
    int res = close(cli_socket);
    if(res < 0) {
        message_log("Failed to close connection from to client", ERR);
        return -1;
    }

    return 0;
}

void create_server_struct(s_tcp_server *srv_out) {
    srv_out->status = UNINITIALIZED;
    srv_out->srv_socket = -1;
}

int make_socket_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        message_log("Failed to get socket flags", ERR);
        return -1;
    }

    flags |= O_NONBLOCK;
    int s = fcntl(fd, F_SETFL, flags);
    if (s == -1) {
        message_log("Failed to set socket flags", ERR);
        return -1;
    }

    return 0;
}

int bind_server_socket(unsigned short port, s_tcp_server *srv_out) {
    //Set socket
    srv_out->srv_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(srv_out->srv_socket < 0) {
        message_log("Failed to initialize server socket", ERR);
        return -1;
    }

    //Set socket properties - use new linux 3.9 API to distribute TCP connections
    //https://lwn.net/Articles/542629/
    int opt = 1;
    if(setsockopt(srv_out->srv_socket, SOL_SOCKET, SO_REUSEPORT, (const char *)&opt,  sizeof(opt)) < 0) {
        message_log("Failed to set sockopt!", ERR);
        return -1;
    }

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(struct sockaddr));

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(port);

    //Bind socket
    int err;
    err = bind(srv_out->srv_socket, (struct sockaddr*)&server_address, sizeof(struct sockaddr));
    if(err < 0) {
        message_log("Bind attempt failed", ERR);
        return -1;
    }

    make_socket_nonblocking(srv_out->srv_socket);

    //Listen on non-blocking socket
    err = listen(srv_out->srv_socket, (int)read_config_long("queueSize", "5"));
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