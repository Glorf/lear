#include "master.h"
#include "config.h"
#include "logger.h"

#include <stdio.h>

int main() {
    init_config("../httpd.yaml");
    init_logger(read_config_string("logPath", "stdout"), read_config_int("logLevel", "0"));
    message_log("Welcome to puthttpd!", DEBUG);
    run_master();

    shutdown_logger();
}