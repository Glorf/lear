//
// Created by mbien on 08.11.18.
//

#ifndef PUTHTTPD_CONFIG_H
#define PUTHTTPD_CONFIG_H

#include "types.h"

typedef struct {
    long max_URI_length;
} s_global_config;

int init_config(char path[]);
s_string read_config_string(char key[], char defaults[]);
long read_config_long(char key[], char defaults[]);
s_global_config *get_global_config();
void init_global_config();

#endif //PUTHTTPD_CONFIG_H
