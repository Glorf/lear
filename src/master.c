#include "master.h"
#include "config.h"
#include "worker.h"
#include "logger.h"

#include <unistd.h>
#include <signal.h>

struct worker {
    int pid;
};

typedef struct worker s_worker;

int run_master() {
    long nworkers = read_config_long("maxNumWorkers", "1");
    s_worker workers[nworkers];

    long numCPU = sysconf(_SC_NPROCESSORS_ONLN); //Only run on max on number of user processors
    if(nworkers > numCPU) nworkers = numCPU;

    //Create workers
    for(long i=0; i<nworkers; i++) {
        workers[i].pid = create_worker();
        if(workers[i].pid > 0)
            message_log("Created new worker", DEBUG);
        else {
            message_log("There was an error during worker creation, aborting...", ERR);
            break;
        }
    }

    //Shutdown master on any interrupt signal
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGTERM);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGKILL);

    /**
     * TODO: make any specific master loop here
     */

    //Wait for shutdown signal
    int result;
    sigwait(&sigset, &result);

    /**
     * We should shutdown workers now
     */
    for(long i=0; i<nworkers; i++) {
        shutdown_worker(workers[i].pid);
        message_log("Shutdown worker", DEBUG);
    }

    return 0;
}

