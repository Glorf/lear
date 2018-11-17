#include "worker.h"
#include "logger.h"
#include "config.h"
#include "connection.h"

#include <unistd.h>
#include <signal.h>
#include <wait.h>
#include <stdlib.h>
#include <sys/epoll.h>

int running;

void stop_worker() {
    running = 0;
}

int create_worker() {
    int pid = fork();

    if(pid > 0)
        return pid;

    /*
     * child zone below
     */

    running = 1;
    message_log("I'm working!", DEBUG);
    signal(SIGTERM, &stop_worker);

    unsigned short port = (unsigned short)read_config_int("listenPort", "9000");

    s_tcp_server server;
    create_server_struct(&server);

    int epoll_fd;

    epoll_fd = epoll_create1(0);
    if(epoll_fd < 0) {
        message_log("Failed to create epoll fd. Worker crashed", ERR);
        exit(-1);
    }

    if(bind_server_socket(port, &server) < 0) {
        message_log("Binding socket failed. Worker crashed", ERR);
        exit(-1);
    }

    struct epoll_event accept_ev;
    accept_ev.data.fd = server.srv_socket;
    accept_ev.events = EPOLLIN | EPOLLET;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server.srv_socket, &accept_ev) < 0) {
        message_log("Failed to add epoll event", ERR);
        running = 0;
    }

    struct epoll_event *event_queue;

    int queueSize = read_config_int("queueSize", "64");
    event_queue = calloc((size_t)queueSize, sizeof(accept_ev));

    while(running) {
        int n = epoll_wait(epoll_fd, event_queue, queueSize, -1);
        for(int i=0; i<n; i++) {
            if ((event_queue[i].events & EPOLLERR) || (event_queue[i].events & EPOLLHUP)) { //queue error
                message_log("Unknown epoll error occured in event queue", WARN);
                close_client_connection(event_queue[i].data.fd);
                continue;
            }
            else if(server.srv_socket == event_queue[i].data.fd) { //incoming connections
                while(accept_client_connection(&server, epoll_fd) != -1);
            }
            else { //there is incoming data from one of connected clients
                while(read_client_connection(event_queue[i].data.fd) != -1);
            }
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