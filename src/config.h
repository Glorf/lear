//
// Created by mbien on 08.11.18.
//

#ifndef PUTHTTPD_CONFIG_H
#define PUTHTTPD_CONFIG_H

#include "types.h"

typedef struct {
    s_string name;
    s_string root_path;
    s_string not_found_path;
} s_host;

typedef struct {
    long max_URI_length;
    long max_request_size;
    long max_block_size;
    long request_timeout_sec;
    //TODO: Handle multiple hosts!
    s_host host;
} s_global_config;



int init_config(char path[]);
s_string read_config_string(char key[], char defaults[]);
long read_config_long(char key[], char defaults[]);
s_global_config *get_global_config();
void init_global_config();

#endif //PUTHTTPD_CONFIG_H
