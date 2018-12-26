//
// Created by mbien on 19.10.18.
//

#ifndef PUTHTTPD_WORKER_H
#define PUTHTTPD_WORKER_H

#include "connection.h"

int create_worker();
int shutdown_worker(int pid);

s_connection *latest;
s_connection *oldest;

#endif //PUTHTTPD_WORKER_H
