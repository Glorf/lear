#include "logger.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

enum LogLevel runVerbosity;
FILE *logfile;

const char *str_levels[4] = {"DEBUG", "INFO", "WARN", "ERR"};

int init_logger(s_string logpath, enum LogLevel logVerbosity) {
    runVerbosity = logVerbosity;
    char *strlogpath = to_c_string(&logpath);
    logfile = fopen(strlogpath, "w");
    free(strlogpath);
    setlinebuf(logfile);
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
        //POSIX claims fprintf is thread safe, added no mutex though
        if(level < ERR) fprintf(logfile, "[%s] %s\n", str_levels[level], message);
        else fprintf(logfile, "[%s] %s: %s\n", str_levels[level], message, strerror(errno));
    }
}

void string_log(s_string *message, enum LogLevel level) {
    if(level >= runVerbosity) {
        //POSIX claims fprintf is thread safe, added no mutex though
        if(level < ERR) fprintf(logfile, "[%s] %.*s\n", str_levels[level], (int)message->length, message->position);
        else fprintf(logfile, "[%s] %.*s: %s\n", str_levels[level], (int)message->length, message->position, strerror(errno));
    }
}