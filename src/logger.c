#include "logger.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

enum LogLevel runVerbosity;

const char *str_levels[4] = {"DEBUG", "INFO", "WARN", "ERR"};

int init_logger(enum LogLevel logVerbosity) {
    runVerbosity = logVerbosity;
    return 0;
}

void message_log(char message[], enum LogLevel level) {
    if(level >= runVerbosity) {
        //POSIX claims fprintf is thread safe, added no mutex though
        if(level < ERR) fprintf(stdout, "[%s] %s\n", str_levels[level], message);
        else fprintf(stderr, "[%s] %s: %s\n", str_levels[level], message, strerror(errno));
    }
}

void string_log(s_string *message, enum LogLevel level) {
    if(level >= runVerbosity) {
        //POSIX claims fprintf is thread safe, added no mutex though
        if(level < ERR) fprintf(stdout, "[%s] %.*s\n", str_levels[level], (int)message->length, message->position);
        else fprintf(stderr, "[%s] %.*s: %s\n", str_levels[level], (int)message->length, message->position, strerror(errno));
    }
}