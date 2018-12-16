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
    event.events = EPOLLIN | EPOLLOUT | EPOLLET;
    s_connection *connection = malloc(sizeof(s_connection));
    connection->fd = cli_socket;

    connection->request_buffer = initialize_buffer();
    connection->response_buffer = initialize_buffer();

    connection->lastAccess = time(NULL);
    connection->currentRequest = NULL;
    connection->requestQueue = 0;

    event.data.ptr = connection;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cli_socket, &event) < 0) {
        message_log("Epoll error adding accepted connection", ERR);
        return -1;
    }
    message_log("New connection processed.", DEBUG);

    return 0;
}

long read_client_connection(s_connection* cli_socket) {
    message_log("Request read started", INFO);

    long max_req = read_config_long("maxRequestSize", "3072");
    long max_block = read_config_long("requestBlockSize", "512");

    ssize_t sum_transmitted = 0;
    ssize_t count = 0;
    while(count != -1) {
        if (cli_socket->request_buffer.size <= cli_socket->request_buffer.offset + 1) { //No space left
            if (cli_socket->request_buffer.size >= max_req) { //request size limit exceeded! BAD REQUEST
                message_log("BAD REQUEST! - is too big", ERR);
                return -1;
            } else { //limit not exceeded yet, expand buffer
                expand_buffer(&cli_socket->request_buffer, max_block);
            }
        }

        count = read(cli_socket->fd,
                cli_socket->request_buffer.payload + cli_socket->request_buffer.offset,
                cli_socket->request_buffer.size - cli_socket->request_buffer.offset);

        if (count == -1 && errno != EAGAIN) {
            message_log("Error while reading from client", ERR);
            return -1;
        } else if(count == -1 && errno == EAGAIN) {
            message_log("Finished this part of request", INFO);
            break;
        } else if(count == 0) {
            message_log("Client disconnected, HANDLE THIS!", WARN);
            return sum_transmitted;
        }

        //Data was read
        cli_socket->request_buffer.offset += count;
        sum_transmitted += count;
        message_log("Data was read", INFO);
    }

    s_string bufferString;
    bufferString.length = cli_socket->request_buffer.offset;
    bufferString.position = cli_socket->request_buffer.payload;

    s_string stopper = create_string("\r\n\r\n", 4);
    s_string bareRequest = substring(&bufferString, &stopper);

    long offset = 0;
    while (bareRequest.length > 0){
        bareRequest.length += 2; //let's save first \r\n for header parsing purposes
        s_http_request *request = parse_request(&bareRequest);

        if(cli_socket->currentRequest == NULL) cli_socket->currentRequest = request;
        else {
            s_http_request *last;
            for (last = cli_socket->currentRequest; last->next != NULL; last = last->next); //get last element of queue
            last->next = request; //append new request at the end of queue
        }
        cli_socket->requestQueue++;

        offset += bareRequest.length+2;

        bufferString.length = cli_socket->request_buffer.offset - offset;
        bufferString.position = cli_socket->request_buffer.payload + offset;

        bareRequest = substring(&bufferString, &stopper);
    }


    //Copy unused part of buffer that potentially holds beggining of next request
    cli_socket->request_buffer.size = cli_socket->request_buffer.offset - offset; //shrink buffer to as small as possible
    char *newbuf = malloc(cli_socket->request_buffer.size);
    memcpy(newbuf, cli_socket->request_buffer.payload+offset, cli_socket->request_buffer.size); //copy existing buffer
    free(cli_socket->request_buffer.payload); //free old buffer
    cli_socket->request_buffer.payload = newbuf; //reassign
    cli_socket->request_buffer.offset = cli_socket->request_buffer.size;

    delete_string(stopper);

    return sum_transmitted;
}

long write_client_connection(s_connection *cli_socket) {
    message_log("Response write started", DEBUG);

    ssize_t sum_transmitted = 0;
    ssize_t count = 0;
    while(count != -1) {
        count = write(cli_socket->fd,
                cli_socket->response_buffer.payload + cli_socket->response_buffer.offset,
                cli_socket->response_buffer.size - cli_socket->response_buffer.offset);

        if (count == -1 && errno != EAGAIN) {
            message_log("Error while writing to client", ERR);
            return -1;
        } else if(count == -1 && errno == EAGAIN) {
            message_log("Client cannot accept any more packets, finishing", INFO);
            break;
        } else if(count == 0) {
            message_log("Client disconnected, HANDLE THIS!", WARN);
            return 0;
        }

        //Data was written
        cli_socket->response_buffer.offset += count;

        if(cli_socket->response_buffer.offset == cli_socket->response_buffer.size) {
            message_log("Buffer is empty, finishing write", INFO);
            //Free buffer if empty
            free(cli_socket->response_buffer.payload);
            cli_socket->response_buffer.size = 0;
            cli_socket->response_buffer.offset = 0;
            break;
        }

        sum_transmitted += count;
        message_log("Data written", INFO);
    }

    return sum_transmitted;
}

int process_client_connection(s_connection *cli_socket){
    message_log("Response being processed", INFO);
    while(cli_socket->requestQueue != 0 && cli_socket->currentRequest != NULL) {
        s_http_request *request = cli_socket->currentRequest;

        s_http_response *response = malloc(sizeof(s_http_response));
        if(process_http_request(cli_socket->currentRequest, response) < 0) { //Main request processing thread
            message_log("Failed to produce response", ERR);
            response->status = INTERNAL_ERROR;
        }

        cli_socket->currentRequest = request->next; //detach request and free it
        cli_socket->requestQueue--;
        free(request);

        message_log("Request fullfilled", INFO);

        s_string headerString = generate_bare_header(response);
        if(headerString.length <= 0) {
            message_log("Failed to serialize server response", ERR);
            return -1;
        }

        /*if(responseString.length > read_config_int("maxResponseSize", "81920000")) {
            message_log("Requested file is too big", ERR);
            //TODO: Should return 500

            return -1;
        }*/

        //TODO: make it non-blocking!!!

        //Reshape buffer to fit the data
        long offset = cli_socket->response_buffer.size;
        long payload_size = headerString.length + response->body_length;
        if(cli_socket->response_buffer.size == 0) cli_socket->response_buffer.payload = malloc((size_t)payload_size);
        else cli_socket->response_buffer.payload = realloc(cli_socket->response_buffer.payload, cli_socket->response_buffer.size+payload_size);
        cli_socket->response_buffer.size += payload_size;

        //Copy new data to buffer
        memcpy(cli_socket->response_buffer.payload+offset,
                headerString.position, headerString.length);
        offset += headerString.length;
        if(response->body_length > 0) {
            memcpy(cli_socket->response_buffer.payload + offset,
                   response->body, response->body_length);
        }


        //Try to write instantly
        write_client_connection(cli_socket);

        /* Write message header */
        //safe_write(cli_socket->fd, headerString.position, headerString.length);

        delete_string(headerString);

        /* Write message body */
        //if(response->body_length>0) safe_write(cli_socket->fd, response->body, response->body_length);
    }

    return 0;
}

void safe_write(int socket, char *data, unsigned long size) {
    long timeout = read_config_long("requestTimeout", "5");
    time_t start = time(NULL);

    size_t sent = 0;

    while(sent < size && time(NULL)-start < timeout) {
        ssize_t s = write(socket, data+sent, size-sent);
        if(s == -1 && errno != EAGAIN) {
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

int close_client_connection(s_connection *cli_socket) {
    int res = close(cli_socket->fd);
    if(res < 0) {
        message_log("Failed to close connection to client", ERR);
        return -1;
    }

    clean_buffer(&cli_socket->request_buffer);
    clean_buffer(&cli_socket->response_buffer);

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