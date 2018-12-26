#include "worker.h"
#include "logger.h"
#include "config.h"
#include "connection.h"

#include <unistd.h>
#include <signal.h>
#include <wait.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <time.h>


int running;

void stop_worker() {
    running = 0;
}

int create_worker() {
    int pid = fork();

    if(pid > 0)
        return pid;
    else if(pid < 0) {
        message_log("Unable to create new worker! Exiting...", ERR);
        return -1;
    }

    /*
     * child zone below
     */

    //Connection linked list for timeouts
    latest = NULL;
    oldest = NULL;

    running = 1;
    message_log("I'm working!", DEBUG);
    signal(SIGTERM, &stop_worker);

    unsigned short port = (unsigned short)read_config_long("listenPort", "9000");

    s_tcp_server server;
    create_server_struct(&server);

    int epoll_fd = epoll_create1(0);
    if(epoll_fd < 0) {
        message_log("Failed to create epoll fd. Worker crashed", ERR);
        exit(-1);
    }

    if(bind_server_socket(port, &server) < 0) {
        message_log("Binding socket failed. Worker crashed", ERR);
        exit(-1);
    }

    struct epoll_event accept_ev;
    s_connection *srv_connection = malloc(sizeof(s_connection));
    srv_connection->fd = server.srv_socket;
    accept_ev.data.ptr = srv_connection;
    accept_ev.events = EPOLLIN | EPOLLET;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server.srv_socket, &accept_ev) < 0) {
        message_log("Failed to add epoll event", ERR);
        running = 0;
    }

    struct epoll_event *event_queue;

    long queueSize = read_config_long("queueSize", "64");
    event_queue = calloc((size_t)queueSize, sizeof(accept_ev));

    while(running) {
        int n = epoll_wait(epoll_fd, event_queue, (int)queueSize, -1);
        //Connection handling logic
        for(int i=0; i<n; i++) {
            if ((event_queue[i].events & EPOLLERR) || (event_queue[i].events & EPOLLHUP)) { //queue error
                message_log("Unknown epoll error occured in event queue", WARN);
                close_client_connection(event_queue[i].data.ptr);
                continue;
            }

            s_connection *cli_connection = event_queue[i].data.ptr;
            if(server.srv_socket == cli_connection->fd) { //incoming connections
                while(accept_client_connection(&server, epoll_fd) != -1);
                continue;
            }
            else if(event_queue[i].events & EPOLLIN) { //there is incoming data from one of connected clients
                //return number of new requests added
                long result = read_client_connection(cli_connection);
                if(result == 0) { //client disconnected, close connection
                    message_log("Client disconnected", INFO);
                    //close connection
                    close_client_connection(cli_connection);
                    continue;
                }
                else if(result == -1) { //invalid request, return 400
                    message_log("BAD REQUEST!", ERR);
                    cli_connection->currentRequest->status = BAD_REQUEST;
                }

                int proc_result = process_client_connection(cli_connection); //process request read
                if(proc_result < 0 && cli_connection->currentRequest != NULL) {
                    message_log("Error 500", ERR);
                    cli_connection->currentRequest->status = BAD_REQUEST;
                }
            }
            else if(event_queue[i].events & EPOLLOUT) { //one of connected clients is ready to read
                if(cli_connection->response_buffer.size > 0) { //there is some data remaining so let's send it
                    long result = write_client_connection(cli_connection);
                    if(result<0) {
                        message_log("Closing connection to client", INFO);
                        //close connection
                        close_client_connection(cli_connection);
                        continue;
                    }
                }
            }

            //Update connection drop timers after request was fullfiled
            cli_connection->drop_timeout = time(NULL) + get_global_config()->request_timeout_sec;
            if(cli_connection != latest) {
                //detach connection from the middle of list
                detach_client_connection(cli_connection);
                //add connection on top of the linked list
                cli_connection->prev = latest;
                if (latest != NULL)
                    latest->next = cli_connection;
                latest = cli_connection;
            }
        }

        //Stale connection dropping logic
        while(oldest != NULL && oldest->drop_timeout < time(NULL)) {
            message_log("Request timeout", INFO);
            if(close_client_connection(oldest) < 0) break;
        }
    }

    close_server_socket(&server);

    exit(0);
}

int shutdown_worker(int pid) {
    int status;
    kill(pid, SIGTERM);
    waitpid(pid, &status, 0);

    if(status < 0) {
        message_log("The child process terminated with error", ERR);
        return -1;
    }

    return 0;
}