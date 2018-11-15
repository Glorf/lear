#include "worker.h"
#include "logger.h"

#include <unistd.h>
#include <signal.h>
#include <wait.h>
#include <stdlib.h>

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
    while(running) {

        /*
         * TODO: run child threads here
         */

        signal(SIGTERM, &stop_worker);
    }

    exit(0);
}

int shutdown_worker(int pid) {
    int status;
    kill(pid, SIGTERM);
    waitpid(pid, &status, 0);

    if(status < 0) message_log("The child process terminated with error", ERR);
}