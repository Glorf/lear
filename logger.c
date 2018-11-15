#include "logger.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>

enum LogLevel runVerbosity;
FILE *logfile;

const char *str_levels[4] = {"DEBUG", "INFO", "WARN", "ERR"};

int init_logger(char logpath[], enum LogLevel logVerbosity) {
    runVerbosity = logVerbosity;
    logfile = fopen(logpath, "w");
    if(logfile == NULL) {
        perror("Failed to initialize log file");
        return -1;
    }
    return 0;
}

int shutdown_logger() {
    return fclose(logfile);
}

void message_log(char message[], enum LogLevel level) {
    if(level >= runVerbosity) {
        if(level < ERR) fprintf(logfile, "[%s] %s\n", str_levels[level], message);
        else fprintf(logfile, "[%s] %s: %s\n", str_levels[level], message, strerror(errno));
    }
}