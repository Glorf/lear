#include "master.h"
#include "config.h"
#include "logger.h"

#include <stdio.h>

int main() {
    run_master();
    init_config("../httpd.yaml"); //TODO: load all below concurrently
    init_logger(read_config_string("logPath", "stdout"), read_config_int("logLevel", "0"));
    message_log("Welcome to puthttpd!", DEBUG);

    shutdown_logger();
}