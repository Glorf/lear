#ifndef PUTHTTPD_LOGGER_H
#define PUTHTTPD_LOGGER_H

enum LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERR = 3
};


int init_logger(char logfile[], enum LogLevel logVerbosity);
void message_log(char message[], enum LogLevel level);
int shutdown_logger();

#endif //PUTHTTPD_LOGGER_H
