#include "master.h"
#include "config.h"
#include "logger.h"

#include <stdio.h>

int main() {
    init_config("httpd.yaml"); //TODO: process log with pipes to master process to omit processing slowdowns
    init_logger(read_config_long("logging.level", "0"));
    message_log("Welcome to LEAR!", DEBUG);
    run_master();

}